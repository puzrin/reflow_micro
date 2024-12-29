#include <gtest/gtest.h>
#include <cstring>
#include "rpc/ble_auth_store.hpp"

// Mock class for IAsyncPreferenceKV
class MockAsyncPreferenceKV : public IAsyncPreferenceKV {
public:
    // Simulate actual storage
    std::map<std::string, std::vector<uint8_t>> storage;

    bool write(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) override {
        std::vector<uint8_t> data(buffer, buffer + length);
        storage[ns + key] = data;
        return true;
    }

    bool read(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) override {
        const auto& data = storage[ns + key];
        std::memcpy(buffer, data.data(), length);
        return true;
    }

    size_t length(const std::string& ns, const std::string& key) override {
        return storage.count(ns + key) ? storage[ns + key].size() : 0;
    }
};

const BleAuthId default_client_id = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
const BleAuthSecret default_secret = { 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };

// Test case: Adding and retrieving clients in BleAuthStore
TEST(BleAuthStoreTest, AddAndRetrieveClient) {
    MockAsyncPreferenceKV kv;
    AsyncPreferenceWriter writer;

    BleAuthStore<4> store(writer, kv, "test");

    // Test that initially client is not present
    EXPECT_FALSE(store.has(default_client_id));

    // Add the client
    store.create(default_client_id, default_secret);

    // Test that client is now present
    EXPECT_TRUE(store.has(default_client_id));

    // Test retrieving the secret
    BleAuthSecret retrieved_secret;
    EXPECT_TRUE(store.get_secret(default_client_id, retrieved_secret));
    EXPECT_EQ(retrieved_secret, default_secret);
}

// Test case: Updating timestamps in BleAuthStore
TEST(BleAuthStoreTest, UpdateTimestamps) {
    MockAsyncPreferenceKV kv;
    AsyncPreferenceWriter writer;

    BleAuthStore<4> store(writer, kv, "test");

    // Add the client
    store.create(default_client_id, default_secret);

    // Set initial timestamp
    uint64_t timestamp = 1625152800;  // Some arbitrary timestamp
    EXPECT_TRUE(store.set_timestamp(default_client_id, timestamp));
    writer.tick(); // Trigger snapshot

    // Test that timestamp was set correctly
    AsyncPreference<std::array<uint64_t, 4>> timestampsPref(writer, kv, "test", "ble_timestamps");
    EXPECT_EQ(timestampsPref.get()[0], timestamp);
}

// Test case: LRU replacement in BleAuthStore
TEST(BleAuthStoreTest, LRUReplacement) {
    MockAsyncPreferenceKV kv;
    AsyncPreferenceWriter writer;

    BleAuthStore<4> store(writer, kv, "test");

    BleAuthId client_id1 = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    BleAuthId client_id2 = { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 };
    BleAuthId client_id3 = { 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3 };
    BleAuthId client_id4 = { 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4 };
    BleAuthId client_id5 = { 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5 };

    // Fill up the store with clients
    store.create(client_id1, default_secret);
    store.create(client_id2, default_secret);
    store.create(client_id3, default_secret);
    store.create(client_id4, default_secret);

    // Add one more client, should replace the least recently used (client_id1)
    store.create(client_id5, default_secret);

    // Test that client_id1 was replaced (LRU)
    EXPECT_FALSE(store.has(client_id1));
    EXPECT_TRUE(store.has(client_id5));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
