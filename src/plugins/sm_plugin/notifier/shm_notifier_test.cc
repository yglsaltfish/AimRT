
#include "shm_notifier.h"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

using namespace aimrt::plugins::sm_plugin;

TEST(ShmNotifierTest, Init) {
  ShmNotifier notifier;
  EXPECT_TRUE(notifier.Init("test_shm_notifier"));
}

TEST(ShmNotifierTest, NotifyAndListen) {
  ShmNotifier notifier1, notifier2;
  EXPECT_TRUE(notifier1.Init("test_shm_notifier"));
  EXPECT_TRUE(notifier2.Init("test_shm_notifier"));

  Notification notification;
  notification.SetHostId(1);
  notification.SetChannelId(2);
  notification.SetBlockIndex(3);
  EXPECT_TRUE(notifier1.Notify(notification));

  Notification received_notification;
  EXPECT_TRUE(notifier2.Listen(&received_notification));
  EXPECT_EQ(received_notification.host_id(), notification.host_id());
  EXPECT_EQ(received_notification.channel_id(), notification.channel_id());
  EXPECT_EQ(received_notification.block_index(), notification.block_index());
}

TEST(ShmNotifierTest, ListenTimeout) {
  ShmNotifier notifier1, notifier2;
  EXPECT_TRUE(notifier1.Init("test_shm_notifier"));

  Notification notification;
  notification.SetHostId(1);
  notification.SetChannelId(2);
  notification.SetBlockIndex(3);

  EXPECT_TRUE(notifier1.Notify(notification));

  EXPECT_TRUE(notifier2.Init("test_shm_notifier"));
  Notification received_notification;
  EXPECT_FALSE(notifier2.Listen(&received_notification, 100));
  EXPECT_EQ(received_notification.host_id(), 0);
  EXPECT_EQ(received_notification.channel_id(), 0);
  EXPECT_EQ(received_notification.block_index(), 0);
}

TEST(ShmNotifierTest, Shutdown) {
  ShmNotifier notifier;
  EXPECT_TRUE(notifier.Init("test_shm_notifier"));

  EXPECT_TRUE(notifier.Shutdown());
}