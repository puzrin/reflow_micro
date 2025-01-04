#include <gtest/gtest.h>
#include "lib/ble_chunker.hpp"

class BleChunkerTest : public ::testing::Test {
protected:
    std::unique_ptr<BleChunker> chunker;

    bool onMessageCalled;
    std::vector<uint8_t> lastReceivedMessage;

    BleChunkerTest() : onMessageCalled(false) {}

    std::vector<uint8_t> onMessageHandler(const std::vector<uint8_t>& message) {
        onMessageCalled = true;
        lastReceivedMessage = message;
        std::vector<uint8_t> response = {'O', 'K'};
        return response;
    }

    virtual void SetUp() override {
        chunker = std::make_unique<BleChunker>();

        chunker->onMessage = [this](const std::vector<uint8_t>& message) {
            return onMessageHandler(message);
        };

        onMessageCalled = false;
        lastReceivedMessage.clear();
        chunker->response.clear();
    }

    std::vector<uint8_t> createChunk(uint8_t messageId, uint16_t sequenceNumber, uint8_t flags, const std::vector<uint8_t>& data) {
        BleChunkHead head(messageId, sequenceNumber, flags);

        std::vector<uint8_t> chunk;
        chunk.resize(BleChunkHead::SIZE);
        head.fillTo(chunk);
        chunk.insert(chunk.end(), data.begin(), data.end());
        return chunk;
    }
};

TEST_F(BleChunkerTest, ChunkAssembly) {
    std::vector<uint8_t> chunk1 = createChunk(1, 0, 0, {'A', 'B', 'C'});
    std::vector<uint8_t> chunk2 = createChunk(1, 1, BleChunkHead::FINAL_CHUNK_FLAG, {'D', 'E', 'F'});

    chunker->consumeChunk(chunk1);
    chunker->consumeChunk(chunk2);

    EXPECT_TRUE(onMessageCalled);
    EXPECT_EQ(static_cast<size_t>(6), lastReceivedMessage.size());
    EXPECT_EQ(std::vector<uint8_t>({'A', 'B', 'C', 'D', 'E', 'F'}), lastReceivedMessage);

    // Check response is correctly assembled
    EXPECT_EQ(static_cast<size_t>(1), chunker->response.size());
}

TEST_F(BleChunkerTest, MessageSizeOverflow) {
    // Create a BleChunker instance with a small maxMessageSize (e.g., 100 bytes)
    BleChunker smallMessageChunker(100);

    std::vector<uint8_t> chunk1 = createChunk(1, 0, 0, std::vector<uint8_t>(50, 0));  // 50 bytes of data
    smallMessageChunker.consumeChunk(chunk1);

    // Create another chunk that will cause the message size to overflow
    std::vector<uint8_t> chunk2 = createChunk(1, 1, BleChunkHead::FINAL_CHUNK_FLAG, std::vector<uint8_t>(51, 0));  // 51 bytes of data
    smallMessageChunker.consumeChunk(chunk2);  // This should cause the overflow

    // Ensure onMessage was not called due to overflow
    //EXPECT_FALSE(onMessageCalled);

    // Check that the response contains an error for message size overflow
    EXPECT_EQ(static_cast<size_t>(1), smallMessageChunker.response.size());

    BleChunkHead errorHead(smallMessageChunker.response[0]);
    EXPECT_EQ(BleChunkHead::SIZE_OVERFLOW_FLAG | BleChunkHead::FINAL_CHUNK_FLAG, errorHead.flags);
}

TEST_F(BleChunkerTest, MissedChunks) {
    std::vector<uint8_t> chunk1 = createChunk(1, 0, 0, {'A', 'B', 'C'});
    std::vector<uint8_t> chunk2 = createChunk(1, 2, BleChunkHead::FINAL_CHUNK_FLAG, {'D', 'E', 'F'});

    chunker->consumeChunk(chunk1);
    chunker->consumeChunk(chunk2);

    EXPECT_FALSE(onMessageCalled);

    // Check response contains an error for missed chunks
    EXPECT_EQ(static_cast<size_t>(1), chunker->response.size());

    BleChunkHead errorHead(chunker->response[0]);
    EXPECT_EQ(BleChunkHead::MISSED_CHUNKS_FLAG | BleChunkHead::FINAL_CHUNK_FLAG, errorHead.flags);
}

TEST_F(BleChunkerTest, MultipleMessages) {
    std::vector<uint8_t> chunk1a = createChunk(1, 0, 0, {'M', 'S', 'G'});
    std::vector<uint8_t> chunk1b = createChunk(1, 1, BleChunkHead::FINAL_CHUNK_FLAG, {'1', 'S', 'T'});

    chunker->consumeChunk(chunk1a);
    chunker->consumeChunk(chunk1b);

    EXPECT_TRUE(onMessageCalled);
    EXPECT_EQ(static_cast<size_t>(6), lastReceivedMessage.size());
    EXPECT_EQ(std::vector<uint8_t>({'M', 'S', 'G', '1', 'S', 'T'}), lastReceivedMessage);
    EXPECT_EQ(static_cast<size_t>(1), chunker->response.size());

    onMessageCalled = false;
    lastReceivedMessage.clear();

    std::vector<uint8_t> chunk2a = createChunk(2, 0, 0, {'2', 'N', 'D'});
    std::vector<uint8_t> chunk2b = createChunk(2, 1, BleChunkHead::FINAL_CHUNK_FLAG, {'M', 'S', 'G'});

    chunker->consumeChunk(chunk2a);
    chunker->consumeChunk(chunk2b);

    EXPECT_TRUE(onMessageCalled);
    EXPECT_EQ(static_cast<size_t>(6), lastReceivedMessage.size());
    EXPECT_EQ(std::vector<uint8_t>({'2', 'N', 'D', 'M', 'S', 'G'}), lastReceivedMessage);
    EXPECT_EQ(static_cast<size_t>(1), chunker->response.size());
}

TEST_F(BleChunkerTest, TooShortChunk) {
    std::vector<uint8_t> chunk1 = createChunk(1, 0, 0, {'A', 'B', 'C'}); // Valid chunk
    std::vector<uint8_t> shortChunk = {0x01, 0x00}; // Too short chunk
    std::vector<uint8_t> chunk2 = createChunk(1, 1, BleChunkHead::FINAL_CHUNK_FLAG, {'D', 'E', 'F'}); // Valid chunk

    chunker->consumeChunk(chunk1);    // First valid chunk
    chunker->consumeChunk(shortChunk); // Short, invalid chunk (should be ignored)
    chunker->consumeChunk(chunk2);    // Second valid chunk

    EXPECT_TRUE(onMessageCalled);
    EXPECT_EQ(static_cast<size_t>(6), lastReceivedMessage.size());
    EXPECT_EQ(std::vector<uint8_t>({'A', 'B', 'C', 'D', 'E', 'F'}), lastReceivedMessage);
    EXPECT_EQ(static_cast<size_t>(1), chunker->response.size());
}

TEST_F(BleChunkerTest, ZeroLengthMessageResponse) {
    chunker->onMessage = [](const std::vector<uint8_t>&) { return std::vector<uint8_t>(); };

    std::vector<uint8_t> chunk1 = createChunk(1, 0, BleChunkHead::FINAL_CHUNK_FLAG, {'A', 'B', 'C'});
    chunker->consumeChunk(chunk1);

    EXPECT_EQ(static_cast<size_t>(1), chunker->response.size());

    BleChunkHead responseHead(chunker->response[0]);
    EXPECT_EQ(BleChunkHead::FINAL_CHUNK_FLAG, responseHead.flags);
    EXPECT_EQ(BleChunkHead::SIZE, chunker->response[0].size()); // Only the header, no data
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}