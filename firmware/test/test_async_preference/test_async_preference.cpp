#include <gtest/gtest.h>
#include <cstring>
#include "lib/async_preference.hpp"

// Mock class for IAsyncPreferenceKV
class MockAsyncPreferenceKV : public IAsyncPreferenceKV {
public:
    // Simulate actual storage
    std::map<std::string, std::vector<uint8_t>> storage;

    void write(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) override {
        std::vector<uint8_t> data(buffer, buffer + length);
        storage[ns + key] = data;
    }

    void read(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) override {
        const auto& data = storage[ns + key];
        std::memcpy(buffer, data.data(), length);
    }

    size_t length(const std::string& ns, const std::string& key) override {
        return storage.count(ns + key) ? storage[ns + key].size() : 0;
    }
};

// Trivially copyable type (int32_t)
TEST(AsyncPreferenceTest, TriviallyCopyable_Int32) {
    MockAsyncPreferenceKV kv;
    AsyncPreferenceWriter pWriter;

    AsyncPreference<int32_t> pref(&pWriter, kv, "ns", "key", -567);

    // Should use default when key not exists
    EXPECT_EQ(kv.length("ns", "key"), size_t(0));
    EXPECT_EQ(pref.get(), -567);

    // Should load from store when key available
    pref.set(123);
    pWriter.tick(); // Should save the snapshot
    EXPECT_EQ(kv.length("ns", "key"), sizeof(int32_t));

    AsyncPreference<int32_t> pref2(nullptr, kv, "ns", "key", -567);
    EXPECT_EQ(pref2.get(), 123);

    // Edge case, write before read
    AsyncPreference<int32_t> pref3(nullptr, kv, "ns", "key");

    pref3.valueUpdateBegin(); // Emulate write start
    EXPECT_EQ(pref3.get(), 0); // Should not read from storage
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
    AsyncPreferenceWriter pWriter;

    AsyncPreference<TestArray> pref(&pWriter, kv, "ns", "key");

    // Elements should be initialized with defaults on create
    EXPECT_EQ(pref.get()[1].bar[2], 3);

    // Should store
    TestArray data = { { { 123, { 4, 5, 6 } }, { 456, { 7, 8, 9 } } } };

    pref.set(data);
    pWriter.tick(); // Trigger snapshot save
    EXPECT_EQ(kv.length("ns", "key"), sizeof(TestArray));

    // Should load from store
    AsyncPreference<TestArray> pref2(&pWriter, kv, "ns", "key");

    EXPECT_EQ(pref2.get()[0].foo, 123);
    EXPECT_EQ(pref2.get()[1].bar[2], 9);

    // Should handle inplace update
    auto& ref = pref2.get();

    pref2.valueUpdateBegin();

    ref[0].foo = 111;
    ref[0].bar[1] = 222;
    ref[1].bar = { 3, 3, 3 };

    pref2.valueUpdateEnd();
    pWriter.tick(); // Trigger snapshot save

    TestArray modified_data = { { { 111, { 4, 222, 6 } }, { 456, { 3, 3, 3 } } } };

    AsyncPreference<TestArray> pref3(nullptr, kv, "ns", "key");
    EXPECT_EQ(pref3.get(), modified_data);
}

// Buffer-like copyable string
TEST(AsyncPreferenceTest, BufferLikeCopyable_String) {
    MockAsyncPreferenceKV kv;
    AsyncPreferenceWriter pWriter;

    // Should init with default constructor
    AsyncPreference<std::string> pref(&pWriter, kv, "ns", "key");
    EXPECT_EQ(pref.get(), "");

    // Should use default when key not exists
    AsyncPreference<std::string> pref2(&pWriter, kv, "ns", "key", "default");
    EXPECT_EQ(kv.length("ns", "key"), size_t(0));
    EXPECT_EQ(pref2.get(), "default");

    // Should load from store when key available
    pref2.set("hello");
    pWriter.tick(); // Should save the snapshot

    AsyncPreference<std::string> pref3(nullptr, kv, "ns", "key");
    EXPECT_EQ(pref3.get(), "hello");
}

// Buffer-like copyable vector
TEST(AsyncPreferenceTest, BufferLikeCopyable_Vector) {
    MockAsyncPreferenceKV kv;
    AsyncPreferenceWriter pWriter;

    // Should init with default constructor
    AsyncPreference<std::vector<uint32_t>> pref(&pWriter, kv, "ns", "key");
    EXPECT_EQ(pref.get(), std::vector<uint32_t>());

    // Should use default when key not exists
    std::vector<uint32_t> default_data = { 1001, 1002, 1003 };
    AsyncPreference<std::vector<uint32_t>> pref2(&pWriter, kv, "ns", "key", default_data);
    EXPECT_EQ(kv.length("ns", "key"), size_t(0));
    EXPECT_EQ(pref2.get(), default_data);

    // Should load from store when key available
    std::vector<uint32_t> data = { 4, 5, 6, 2000 }; // new content & size
    pref2.set(data);
    pWriter.tick(); // Should save the snapshot

    AsyncPreference<std::vector<uint32_t>> pref3(nullptr, kv, "ns", "key");
    EXPECT_EQ(pref3.get(), data);
}

// Map of trivially copyable values
TEST(AsyncPreferenceTest, Map_TriviallyCopyableValues) {
    MockAsyncPreferenceKV kv;
    AsyncPreferenceWriter pWriter;

    AsyncPreferenceMap<int32_t> map(&pWriter, kv, "ns", "key", -1);

    // Should use default when item not exists
    EXPECT_EQ(map[5].get(), -1);

    // Should store value
    map[5].set(123);
    pWriter.tick();
    EXPECT_EQ(kv.length("ns", "key_5"), sizeof(int32_t));

    // Should load from store
    AsyncPreferenceMap<int32_t> map2(nullptr, kv, "ns", "key", -1);
    EXPECT_EQ(map2[5].get(), 123);
    EXPECT_EQ(map2[6].get(), -1);  // Other indices still return default
}

// Map of string values
TEST(AsyncPreferenceTest, Map_StringValues) {
    MockAsyncPreferenceKV kv;
    AsyncPreferenceWriter pWriter;

    AsyncPreferenceMap<std::string> map(&pWriter, kv, "ns", "key", "default");
    map[1].set("hello");
    map[42].set("world");
    pWriter.tick();

    AsyncPreferenceMap<std::string> map2(nullptr, kv, "ns", "key", "default");
    EXPECT_EQ(map2[1].get(), "hello");
    EXPECT_EQ(map2[42].get(), "world");
    EXPECT_EQ(map2[2].get(), "default");
}

// Map of vector values
TEST(AsyncPreferenceTest, Map_VectorValues) {
    MockAsyncPreferenceKV kv;
    AsyncPreferenceWriter pWriter;

    std::vector<int32_t> v1 = {1000, 1001, 10002};
    std::vector<int32_t> v2 = {2010, 2011};
    std::vector<int32_t> v_default = {-1, -2};

    AsyncPreferenceMap<std::vector<int32_t>> map(&pWriter, kv, "ns", "key", v_default);
    map[1].set(v1);
    map[42].set(v2);
    pWriter.tick();

    AsyncPreferenceMap<std::vector<int32_t>> map2(nullptr, kv, "ns", "key", v_default);
    EXPECT_EQ(map2[1].get(), v1);
    EXPECT_EQ(map2[42].get(), v2);
    EXPECT_EQ(map2[2].get(), v_default);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
