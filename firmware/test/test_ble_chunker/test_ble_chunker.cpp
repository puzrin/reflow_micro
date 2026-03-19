#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "lib/ble_chunker.hpp"

namespace {

static constexpr size_t TEST_MAX_MESSAGE_SIZE = 64;
using TestChunker = BleChunker<TEST_MAX_MESSAGE_SIZE>;

auto createChunk(uint8_t messageId, uint16_t sequenceNumber, uint8_t flags, const std::vector<uint8_t>& data)
    -> std::vector<uint8_t>
{
    BleChunkHead head(messageId, sequenceNumber, flags);

    std::vector<uint8_t> chunk(BleChunkHead::SIZE);
    head.fillTo(chunk.data());
    chunk.insert(chunk.end(), data.begin(), data.end());
    return chunk;
}

template <size_t MaxMessageSize>
auto readResponseChunk(BleChunker<MaxMessageSize>& chunker) -> std::vector<uint8_t> {
    const auto view = chunker.getResponseChunk();
    return std::vector<uint8_t>(view.data, view.data + view.size);
}

class BleChunkerTest : public ::testing::Test {
public:
    std::unique_ptr<TestChunker> chunker;
    bool onMessageCalled{false};
    std::vector<uint8_t> lastReceivedMessage;

    void SetUp() override {
        chunker = std::make_unique<TestChunker>();
        chunker->setMessageHandler(TestChunker::MessageHandler::create<BleChunkerTest, &BleChunkerTest::onMessageHandler>(*this));
        onMessageCalled = false;
        lastReceivedMessage.clear();
    }

    void onMessageHandler(const TestChunker::MessageBuffer& message, TestChunker::MessageBuffer& response) {
        onMessageCalled = true;
        lastReceivedMessage.assign(message.begin(), message.end());
        response.clear();
        response.push_back('O');
        response.push_back('K');
    }

    void onEmptyResponseHandler(const TestChunker::MessageBuffer& message, TestChunker::MessageBuffer& response) {
        onMessageCalled = true;
        lastReceivedMessage.assign(message.begin(), message.end());
        response.clear();
    }
};

TEST_F(BleChunkerTest, ChunkAssembly) {
    const auto chunk1 = createChunk(1, 0, 0, {'A', 'B', 'C'});
    const auto chunk2 = createChunk(1, 1, BleChunkHead::FINAL_CHUNK_FLAG, {'D', 'E', 'F'});

    chunker->consumeChunk(chunk1.data(), chunk1.size());
    chunker->consumeChunk(chunk2.data(), chunk2.size());

    EXPECT_TRUE(onMessageCalled);
    EXPECT_EQ((std::vector<uint8_t>{'A', 'B', 'C', 'D', 'E', 'F'}), lastReceivedMessage);

    const auto response = readResponseChunk(*chunker);
    ASSERT_EQ(BleChunkHead::SIZE + 2, response.size());
    EXPECT_EQ('O', response[BleChunkHead::SIZE]);
    EXPECT_EQ('K', response[BleChunkHead::SIZE + 1]);
    EXPECT_EQ(BleChunkHead::FINAL_CHUNK_FLAG, BleChunkHead(response.data()).flags);
}

TEST(BleChunkerStandaloneTest, MessageSizeOverflow) {
    BleChunker<8> chunker;

    const auto chunk1 = createChunk(1, 0, 0, {0, 0, 0, 0, 0});
    const auto chunk2 = createChunk(1, 1, BleChunkHead::FINAL_CHUNK_FLAG, {0, 0, 0, 0});

    chunker.consumeChunk(chunk1.data(), chunk1.size());
    chunker.consumeChunk(chunk2.data(), chunk2.size());

    const auto response = readResponseChunk(chunker);
    ASSERT_EQ(BleChunkHead::SIZE, response.size());
    EXPECT_EQ(
        BleChunkHead::SIZE_OVERFLOW_FLAG | BleChunkHead::FINAL_CHUNK_FLAG,
        BleChunkHead(response.data()).flags
    );
}

TEST_F(BleChunkerTest, MissedChunks) {
    const auto chunk1 = createChunk(1, 0, 0, {'A', 'B', 'C'});
    const auto chunk2 = createChunk(1, 2, BleChunkHead::FINAL_CHUNK_FLAG, {'D', 'E', 'F'});

    chunker->consumeChunk(chunk1.data(), chunk1.size());
    chunker->consumeChunk(chunk2.data(), chunk2.size());

    EXPECT_FALSE(onMessageCalled);

    const auto response = readResponseChunk(*chunker);
    ASSERT_EQ(BleChunkHead::SIZE, response.size());
    EXPECT_EQ(
        BleChunkHead::MISSED_CHUNKS_FLAG | BleChunkHead::FINAL_CHUNK_FLAG,
        BleChunkHead(response.data()).flags
    );
}

TEST_F(BleChunkerTest, MultipleMessages) {
    const auto chunk1a = createChunk(1, 0, 0, {'M', 'S', 'G'});
    const auto chunk1b = createChunk(1, 1, BleChunkHead::FINAL_CHUNK_FLAG, {'1', 'S', 'T'});

    chunker->consumeChunk(chunk1a.data(), chunk1a.size());
    chunker->consumeChunk(chunk1b.data(), chunk1b.size());

    EXPECT_TRUE(onMessageCalled);
    EXPECT_EQ((std::vector<uint8_t>{'M', 'S', 'G', '1', 'S', 'T'}), lastReceivedMessage);
    EXPECT_EQ(BleChunkHead::FINAL_CHUNK_FLAG, BleChunkHead(readResponseChunk(*chunker).data()).flags);

    onMessageCalled = false;
    lastReceivedMessage.clear();

    const auto chunk2a = createChunk(2, 0, 0, {'2', 'N', 'D'});
    const auto chunk2b = createChunk(2, 1, BleChunkHead::FINAL_CHUNK_FLAG, {'M', 'S', 'G'});

    chunker->consumeChunk(chunk2a.data(), chunk2a.size());
    chunker->consumeChunk(chunk2b.data(), chunk2b.size());

    EXPECT_TRUE(onMessageCalled);
    EXPECT_EQ((std::vector<uint8_t>{'2', 'N', 'D', 'M', 'S', 'G'}), lastReceivedMessage);
    EXPECT_EQ(BleChunkHead::FINAL_CHUNK_FLAG, BleChunkHead(readResponseChunk(*chunker).data()).flags);
}

TEST_F(BleChunkerTest, TooShortChunk) {
    const auto chunk1 = createChunk(1, 0, 0, {'A', 'B', 'C'});
    const std::vector<uint8_t> shortChunk = {0x01, 0x00};
    const auto chunk2 = createChunk(1, 1, BleChunkHead::FINAL_CHUNK_FLAG, {'D', 'E', 'F'});

    chunker->consumeChunk(chunk1.data(), chunk1.size());
    chunker->consumeChunk(shortChunk.data(), shortChunk.size());
    chunker->consumeChunk(chunk2.data(), chunk2.size());

    EXPECT_TRUE(onMessageCalled);
    EXPECT_EQ((std::vector<uint8_t>{'A', 'B', 'C', 'D', 'E', 'F'}), lastReceivedMessage);
}

TEST_F(BleChunkerTest, ZeroLengthMessageResponse) {
    chunker->setMessageHandler(TestChunker::MessageHandler::create<BleChunkerTest, &BleChunkerTest::onEmptyResponseHandler>(*this));

    const auto chunk = createChunk(1, 0, BleChunkHead::FINAL_CHUNK_FLAG, {'A', 'B', 'C'});
    chunker->consumeChunk(chunk.data(), chunk.size());

    const auto response = readResponseChunk(*chunker);
    ASSERT_EQ(BleChunkHead::SIZE, response.size());
    EXPECT_EQ(BleChunkHead::FINAL_CHUNK_FLAG, BleChunkHead(response.data()).flags);
}

TEST_F(BleChunkerTest, ReturnsNoDataWhenNoResponseReady) {
    const auto response = readResponseChunk(*chunker);
    ASSERT_EQ(1u, response.size());
    EXPECT_EQ(0u, response[0]);
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
