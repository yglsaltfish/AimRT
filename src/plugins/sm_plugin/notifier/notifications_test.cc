
#include "notifications.h"
#include <gtest/gtest.h>

using namespace aimrt::plugins::sm_plugin;

TEST(NotificationTest, Constructor) {
  Notification notification;
  EXPECT_FALSE(notification);

  Notification notification2(123, 456, 789);
  EXPECT_TRUE(notification2);
  EXPECT_EQ(notification2.host_id(), 123);
  EXPECT_EQ(notification2.channel_id(), 456);
  EXPECT_EQ(notification2.block_index(), 789);
}

TEST(NotificationTest, SetValue) {
  Notification notification;
  EXPECT_FALSE(notification);
  notification.SetHostId(123);
  notification.SetChannelId(456);
  notification.SetBlockIndex(789);
  EXPECT_TRUE(notification);
  EXPECT_EQ(notification.host_id(), 123);
  EXPECT_EQ(notification.channel_id(), 456);
  EXPECT_EQ(notification.block_index(), 789);
}

TEST(NotificationTest, CopyConstructor) {
  Notification notification(123, 456, 789);
  Notification notification2(notification);
  EXPECT_TRUE(notification2);
  EXPECT_EQ(notification2.host_id(), 123);
  EXPECT_EQ(notification2.channel_id(), 456);
  EXPECT_EQ(notification2.block_index(), 789);
}

TEST(NotificationTest, AssignmentOperator) {
  Notification notification(123, 456, 789);
  Notification notification2;
  notification2 = notification;
  EXPECT_TRUE(notification2);
  EXPECT_EQ(notification2.host_id(), 123);
  EXPECT_EQ(notification2.channel_id(), 456);
  EXPECT_EQ(notification2.block_index(), 789);
}

TEST(NotificationTest, EqualityOperator) {
  Notification notification(123, 456, 789);
  Notification notification2(123, 456, 789);
  Notification notification3(123, 456, 790);
  EXPECT_TRUE(notification == notification2);
  EXPECT_FALSE(notification == notification3);
}

TEST(NotificationTest, InequalityOperator) {
  Notification notification(123, 456, 789);
  Notification notification2(123, 456, 789);
  Notification notification3(123, 456, 790);
  EXPECT_FALSE(notification != notification2);
  EXPECT_TRUE(notification != notification3);
}

TEST(NotificationTest, Serialization) {
  Notification notification(123, 456, 789);
  std::string output;
  EXPECT_TRUE(notification.Serialize(&output));
  Notification notification2;
  EXPECT_TRUE(notification2.Deserialize(output));
  EXPECT_TRUE(notification == notification2);

  Notification notification3;
  EXPECT_FALSE(notification3.Deserialize("invalid data"));
}

TEST(NotificationTest, Deserialize) {
  Notification notification(123, 456, 789);
  std::string output;
  EXPECT_TRUE(notification.Serialize(&output));

  Notification notification2;
  EXPECT_FALSE(notification2.Deserialize("invalid data"));

  Notification notification3;
  EXPECT_TRUE(notification3.Deserialize(output.data(), output.size()));

  Notification notification4;
  EXPECT_FALSE(notification4.Deserialize(output.data(), output.size() - 1));
}
