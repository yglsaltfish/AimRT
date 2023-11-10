

#include "block.h"
#include <gtest/gtest.h>

using namespace aimrt::plugins::sm_plugin;

TEST(BlockTest, SetMsgSize) {
  Block block;
  block.SetMsgSize(42);
  EXPECT_EQ(block.msg_size(), 42);
}

// TEST(BlockTest, TryLockForWrite) {
//   Block block;
//   EXPECT_TRUE(block.TryLockForWrite());
//   EXPECT_FALSE(block.TryLockForWrite());
// }

// TEST(BlockTest, TryLockForRead) {
//   Block block;
//   EXPECT_TRUE(block.TryLockForRead());
//   EXPECT_TRUE(block.TryLockForRead());
//   EXPECT_TRUE(block.TryLockForRead());
//   EXPECT_TRUE(block.TryLockForRead());
//   EXPECT_TRUE(block.TryLockForRead());
//   EXPECT_TRUE(block.TryLockForRead());
//   block.ReleaseReadLock();
// }

// TEST(BlockTest, TryLockForReadWhileWriteLock) {
//   Block block;
//   EXPECT_TRUE(block.TryLockForWrite());
//   EXPECT_FALSE(block.TryLockForRead());
//   block.ReleaseWriteLock();
//   EXPECT_TRUE(block.TryLockForRead());
//   block.ReleaseReadLock();
// }

// TEST(BlockTest, ReleaseWriteLock) {
//   Block block;
//   EXPECT_TRUE(block.TryLockForWrite());
//   EXPECT_FALSE(block.TryLockForWrite());
//   block.ReleaseWriteLock();
//   EXPECT_TRUE(block.TryLockForWrite());
// }

// TEST(BlockTest, ReleaseReadLock) {
//   Block block;
//   EXPECT_TRUE(block.TryLockForRead());
//   EXPECT_TRUE(block.TryLockForRead());
//   block.ReleaseReadLock();
//   EXPECT_TRUE(block.TryLockForRead());
//   block.ReleaseReadLock();
// }
