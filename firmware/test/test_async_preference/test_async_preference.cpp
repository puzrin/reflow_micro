#include <gtest/gtest.h>
#include <cstring>
#include "lib/async_preference.hpp"

// Mock class for IAsyncPreferenceKV
class MockAsyncPreferenceKV : public IAsyncPreferenceKV {
public:
    // Simulate actual storage
    std::map<std::string, std::vector<uint8_t>> storage;

    bool write(etl::string_view ns, etl::string_view key, uint8_t* buffer, size_t length) override {
        std::vector<uint8_t> data(buffer, buffer + length);
        storage[std::string(ns.data(), ns.size()) + std::string(key.data(), key.size())] = data;
        return true;
    }

    bool read(etl::string_view ns, etl::string_view key, uint8_t* buffer, size_t length) override {
        const auto& data = storage[std::string(ns.data(), ns.size()) + std::string(key.data(), key.size())];
        std::memcpy(buffer, data.data(), length);
        return true;
    }

    size_t length(etl::string_view ns, etl::string_view key) override {
        const auto storage_key = std::string(ns.data(), ns.size()) + std::string(key.data(), key.size());
        return storage.count(storage_key) ? storage[storage_key].size() : 0;
    }
};

// Trivially copyable type (int32_t)
TEST(AsyncPreferenceTest, TriviallyCopyable_Int32) {
    MockAsyncPreferenceKV kv;
    AsyncPreferenceWriter Writer;

    AsyncPreference<int32_t> pref(Writer, kv, "ns", "key", -567);

    // Should use default when key not exists
    EXPECT_EQ(kv.length("ns", "key"), size_t(0));
    EXPECT_EQ(pref.get(), -567);

    // Should load from store when key available
    pref.set(123);
    Writer.tick(); // Should save the snapshot
    EXPECT_EQ(kv.length("ns", "key"), sizeof(int32_t));

    AsyncPreference<int32_t> pref2(Writer, kv, "ns", "key", -567);
    EXPECT_EQ(pref2.get(), 123);

    // Edge case, write before read (not actual with forced preload)
    //AsyncPreference<int32_t> pref3(Writer, kv, "ns", "key");
    //pref3.valueUpdateBegin(); // Emulate write start
    //EXPECT_EQ(pref3.get(), 0); // Should not read from storage
}

struct TestStruct {
    int32_t foo = 15;
    std::array<uint8_t, 3> bar = {1, 2, 3};

    bool operator==(const TestStruct& other) const {
        return foo == other.foo && bar == other.bar;
    }
};

using TestArray = std::array<TestStruct, 2>;

// Trivially copyable fixed array
TEST(AsyncPreferenceTest, TriviallyCopyable_ArrayOfStructs) {
    MockAsyncPreferenceKV kv;
    AsyncPreferenceWriter writer;

    AsyncPreference<TestArray> pref(writer, kv, "ns", "key");

    // Elements should be initialized with defaults on create
    EXPECT_EQ(pref.get()[1].bar[2], 3);

    // Should store
    TestArray data = { { { 123, { 4, 5, 6 } }, { 456, { 7, 8, 9 } } } };

    pref.set(data);
    writer.tick(); // Trigger snapshot save
    EXPECT_EQ(kv.length("ns", "key"), sizeof(TestArray));

    // Should load from store
    AsyncPreference<TestArray> pref2(writer, kv, "ns", "key");

    EXPECT_EQ(pref2.get()[0].foo, 123);
    EXPECT_EQ(pref2.get()[1].bar[2], 9);

    // Should handle inplace update
    auto& ref = pref2.get();

    pref2.valueUpdateBegin();

    ref[0].foo = 111;
    ref[0].bar[1] = 222;
    ref[1].bar = { 3, 3, 3 };

    pref2.valueUpdateEnd();
    writer.tick(); // Trigger snapshot save

    TestArray modified_data = { { { 111, { 4, 222, 6 } }, { 456, { 3, 3, 3 } } } };

    AsyncPreference<TestArray> pref3(writer, kv, "ns", "key");
    EXPECT_EQ(pref3.get(), modified_data);
}

// Buffer-like copyable string
TEST(AsyncPreferenceTest, BufferLikeCopyable_String) {
    MockAsyncPreferenceKV kv;
    AsyncPreferenceWriter writer;

    // Should init with default constructor
    AsyncPreference<std::string> pref(writer, kv, "ns", "key");
    EXPECT_EQ(pref.get(), "");

    // Should use default when key not exists
    AsyncPreference<std::string> pref2(writer, kv, "ns", "key", "default");
    EXPECT_EQ(kv.length("ns", "key"), size_t(0));
    EXPECT_EQ(pref2.get(), "default");

    // Should load from store when key available
    pref2.set("hello");
    writer.tick(); // Should save the snapshot

    AsyncPreference<std::string> pref3(writer, kv, "ns", "key");
    EXPECT_EQ(pref3.get(), "hello");
}

// Buffer-like copyable vector
TEST(AsyncPreferenceTest, BufferLikeCopyable_Vector) {
    MockAsyncPreferenceKV kv;
    AsyncPreferenceWriter writer;

    // Should init with default constructor
    AsyncPreference<std::vector<uint32_t>> pref(writer, kv, "ns", "key");
    EXPECT_EQ(pref.get(), std::vector<uint32_t>());

    // Should use default when key not exists
    std::vector<uint32_t> default_data = { 1001, 1002, 1003 };
    AsyncPreference<std::vector<uint32_t>> pref2(writer, kv, "ns", "key", default_data);
    EXPECT_EQ(kv.length("ns", "key"), size_t(0));
    EXPECT_EQ(pref2.get(), default_data);

    // Should load from store when key available
    std::vector<uint32_t> data = { 4, 5, 6, 2000 }; // new content & size
    pref2.set(data);
    writer.tick(); // Should save the snapshot

    AsyncPreference<std::vector<uint32_t>> pref3(writer, kv, "ns", "key");
    EXPECT_EQ(pref3.get(), data);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
