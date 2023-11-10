

#include "status_manager.h"
#include <gtest/gtest.h>

using namespace aimrt::plugins::sm_plugin;

TEST(StatusManagerTest, DecreaseReferenceCountsTest) {
  StatusManager sm(10);
  sm.IncreaseReferenceCounts();
  sm.IncreaseReferenceCounts();
  sm.DecreaseReferenceCounts();
  EXPECT_EQ(sm.reference_count(), 1);
}

TEST(StatusManagerTest, IncreaseReferenceCountsTest) {
  StatusManager sm(10);
  sm.IncreaseReferenceCounts();
  sm.IncreaseReferenceCounts();
  EXPECT_EQ(sm.reference_count(), 2);
}

TEST(StatusManagerTest, SetNeedRemapTest) {
  StatusManager sm(10);
  sm.SetNeedRemap(true);
  EXPECT_TRUE(sm.need_map());
}

TEST(StatusManagerTest, FetchAddSeqTest) {
  StatusManager sm(10);
  EXPECT_EQ(sm.FetchAddSeq(5), 0);
  EXPECT_EQ(sm.seq(), 5);
}

TEST(StatusManagerTest, CeilingMsgSizeTest) {
  StatusManager sm(10);
  EXPECT_EQ(sm.ceiling_msg_size(), 10);
}