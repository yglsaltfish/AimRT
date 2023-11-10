
#include "context.h"
#include <gtest/gtest.h>

using namespace aimrt::plugins::sm_plugin;

TEST(ContextTest, DefaultConstructor) {
  Context context;
  EXPECT_EQ(context.seq(), 0);
  EXPECT_EQ(context.sender_id(), 0);
  EXPECT_EQ(context.channel_id(), 0);
  EXPECT_EQ(context.timestamp(), 0);
}

TEST(ContextTest, CopyConstructor) {
  Context context1;
  context1.SetSeq(123);
  context1.SetSenderId(456);
  context1.SetChannelId(789);
  context1.SetTimestamp(1000);

  Context context2(context1);
  EXPECT_EQ(context2.seq(), 123);
  EXPECT_EQ(context2.sender_id(), 456);
  EXPECT_EQ(context2.channel_id(), 789);
  EXPECT_EQ(context2.timestamp(), 1000);
}

TEST(ContextTest, AssignmentOperator) {
  Context context1;
  context1.SetSeq(123);
  context1.SetSenderId(456);
  context1.SetChannelId(789);
  context1.SetTimestamp(1000);

  Context context2;
  context2 = context1;
  EXPECT_EQ(context2.seq(), 123);
  EXPECT_EQ(context2.sender_id(), 456);
  EXPECT_EQ(context2.channel_id(), 789);
  EXPECT_EQ(context2.timestamp(), 1000);
}

TEST(ContextTest, EqualityOperator) {
  Context context1;
  context1.SetSeq(123);
  context1.SetSenderId(456);
  context1.SetChannelId(789);
  context1.SetTimestamp(1000);

  Context context2(context1);
  EXPECT_TRUE(context1 == context2);

  context2.SetSeq(456);
  EXPECT_FALSE(context1 == context2);
}

TEST(ContextTest, InequalityOperator) {
  Context context1;
  context1.SetSeq(123);
  context1.SetSenderId(456);
  context1.SetChannelId(789);
  context1.SetTimestamp(1000);

  Context context2(context1);
  EXPECT_FALSE(context1 != context2);

  context2.SetSeq(456);
  EXPECT_TRUE(context1 != context2);
}
