#include <gtest/gtest.h>
#include "utils/data_guard.hpp"
#include <string>

TEST(DataGuardTest, SnapshotFailsWithoutWrite) {
    DataGuard<int> dataGuard(5);

    // Snapshot should fail without prior write
    bool success = dataGuard.makeSnapshot();
    EXPECT_FALSE(success);
}

TEST(DataGuardTest, SnapshotUpdatesAfterWrite) {
    DataGuard<std::string> dataGuard;

    dataGuard.writeData("first");
    bool success = dataGuard.makeSnapshot();
    ASSERT_TRUE(success);
    EXPECT_EQ(dataGuard.snapshot, "first");

    dataGuard.writeData("second");
    success = dataGuard.makeSnapshot();
    ASSERT_TRUE(success);
    EXPECT_EQ(dataGuard.snapshot, "second");
}

TEST(DataGuardTest, SnapshotStaysSameWithoutNewWrite) {
    DataGuard<int> dataGuard(0);

    dataGuard.writeData(5);
    bool success = dataGuard.makeSnapshot();
    ASSERT_TRUE(success);
    EXPECT_EQ(dataGuard.snapshot, 5);

    // Snapshot should remain the same without new write
    success = dataGuard.makeSnapshot();
    EXPECT_FALSE(success);
    EXPECT_EQ(dataGuard.snapshot, 5);
}

TEST(DataGuardTest, TransactionBlocksSnapshot) {
    DataGuard<std::string> dataGuard;

    dataGuard.writeData("first");

    dataGuard.beginWrite();

    bool success = dataGuard.makeSnapshot();
    ASSERT_FALSE(success);

    dataGuard.value[1] = 'a';
    dataGuard.endWrite();

    success = dataGuard.makeSnapshot();
    ASSERT_TRUE(success);
    EXPECT_EQ(dataGuard.snapshot, "farst");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
