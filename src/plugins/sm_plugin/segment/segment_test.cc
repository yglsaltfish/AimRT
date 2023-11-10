
#include "segment.h"
#include "gtest/gtest.h"

using namespace aimrt::plugins::sm_plugin;

std::shared_ptr<Segment> segmentWritePtr{nullptr};
std::shared_ptr<Segment> segmentReadPtr{nullptr};

TEST(SegmentTest, AcquireBlockForWrite) {
  if (!segmentWritePtr) {
    segmentWritePtr = std::make_shared<Segment>("test_segment");
  }
  WritableBlock writable_block;
  ASSERT_TRUE(segmentWritePtr->TryAcquireBlockForWrite(1024, &writable_block));
  ASSERT_TRUE(writable_block.block != nullptr);
  ASSERT_TRUE(writable_block.context != nullptr);
  ASSERT_TRUE(writable_block.buf != nullptr);
  ASSERT_EQ(writable_block.index, 0);
  memcpy(writable_block.buf, "hello world", 11);
  writable_block.block->SetMsgSize(11);
  segmentWritePtr->ReleaseWriteBlock(writable_block);
  ASSERT_TRUE(segmentWritePtr->IntegrityCheck());
}

TEST(SegmentTest, AcquireBlockForRead) {
  if (!segmentReadPtr) {
    segmentReadPtr = std::make_shared<Segment>("test_segment");
  }
  ReadableBlock readable_block;
  ASSERT_TRUE(segmentWritePtr->IntegrityCheck());
  ASSERT_TRUE((segmentReadPtr->TryAcquireBlockForRead(&readable_block)));
  ASSERT_TRUE(readable_block.block != nullptr);
  ASSERT_TRUE(readable_block.context != nullptr);
  ASSERT_TRUE(readable_block.buf != nullptr);
  ASSERT_EQ(readable_block.block->msg_size(), 11);
  ASSERT_TRUE(memcmp(readable_block.buf, "hello world", 11) == 0);
  ASSERT_EQ(readable_block.index, 0);
  segmentReadPtr->ReleaseReadBlock(readable_block);
  ASSERT_TRUE(segmentWritePtr->IntegrityCheck());
}

TEST(SegmentTest, AcquireBlockForWriteMultiple) {
  Segment segment("test_segment_multiple");
  WritableBlock writable_block;
  ASSERT_TRUE(segment.TryAcquireBlockForWrite(1024, &writable_block));
  ASSERT_TRUE(writable_block.block != nullptr);
  ASSERT_TRUE(writable_block.context != nullptr);
  ASSERT_TRUE(writable_block.buf != nullptr);
  ASSERT_EQ(writable_block.index, 0);
  memcpy(writable_block.buf, "hello world", 11);
  segment.ReleaseWriteBlock(writable_block);
  ASSERT_TRUE(segment.IntegrityCheck());
  ASSERT_TRUE(segment.TryAcquireBlockForWrite(1024, &writable_block));
  ASSERT_EQ(writable_block.index, 1);
  ASSERT_TRUE(segment.TryAcquireBlockForWrite(1024, &writable_block));
  ASSERT_EQ(writable_block.index, 2);
  ASSERT_TRUE(segment.TryAcquireBlockForWrite(1024, &writable_block));
  ASSERT_TRUE(segment.TryAcquireBlockForWrite(1024, &writable_block));
  ASSERT_TRUE(segment.TryAcquireBlockForWrite(1024, &writable_block));
  ASSERT_EQ(writable_block.index, 5);
  ASSERT_TRUE(segment.IntegrityCheck());
}

TEST(SegmentTest, ReCreate) {
  Segment segment("test_segment_recreate");
  WritableBlock writable_block;
  ASSERT_TRUE(segment.TryAcquireBlockForWrite(1024, &writable_block));
  ASSERT_TRUE(writable_block.block != nullptr);
  ASSERT_TRUE(writable_block.context != nullptr);
  ASSERT_TRUE(writable_block.buf != nullptr);
  ASSERT_EQ(writable_block.index, 0);
  memcpy(writable_block.buf, "hello world", 11);
  segment.ReleaseWriteBlock(writable_block);

  ASSERT_TRUE(segment.TryAcquireBlockForWrite(1024 * 20, &writable_block));
  ASSERT_TRUE(writable_block.block != nullptr);
  ASSERT_TRUE(writable_block.context != nullptr);
  ASSERT_TRUE(writable_block.buf != nullptr);
  ASSERT_EQ(writable_block.index, 0);
  memcpy(writable_block.buf, "hello world", 11);
  segment.ReleaseWriteBlock(writable_block);
}

TEST(SegmentTest, ReleaseWriteBlockTwice) {
  Segment segment("test_segment_twice");
  WritableBlock writable_block;
  ASSERT_TRUE(segment.TryAcquireBlockForWrite(1024, &writable_block));
  ASSERT_TRUE(writable_block.block != nullptr);
  ASSERT_TRUE(writable_block.context != nullptr);
  ASSERT_TRUE(writable_block.buf != nullptr);
  ASSERT_EQ(writable_block.index, 0);
  segment.ReleaseWriteBlock(writable_block);
  segment.ReleaseWriteBlock(writable_block);
}

TEST(SegmentTest, PrintInfo) {
  Segment segment("test_segment_print");
  WritableBlock writable_block;
  ASSERT_TRUE(segment.TryAcquireBlockForWrite(1024 * 1024 * 64, &writable_block));
  segment.PrintInfo();
}
