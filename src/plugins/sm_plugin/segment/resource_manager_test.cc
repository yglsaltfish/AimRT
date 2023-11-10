
#include "resource_manager.h"
#include "block.h"
#include "context.h"

#include <gtest/gtest.h>

TEST(ResourceManagerTest, UpdateMsgSizeDefault) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 2);
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 2);
  EXPECT_EQ(manager.real_block_size(), 1024 * 2);
  EXPECT_EQ(manager.block_count(), 1024);
  EXPECT_EQ(manager.context_size(), sizeof(aimrt::plugins::sm_plugin::Context));
}

TEST(ResourceManagerTest, UpdateMsgSize100B) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  manager.UpdateMsgSize(100);
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 2);
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 2);
  EXPECT_EQ(manager.real_block_size(), 1024 * 2);
  EXPECT_EQ(manager.block_count(), 1024);
  EXPECT_EQ(manager.context_size(), sizeof(aimrt::plugins::sm_plugin::Context));
}

TEST(ResourceManagerTest, UpdateMsgSize100K) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  manager.UpdateMsgSize(1024 * 100);
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 128);
  EXPECT_EQ(manager.real_block_size(), 1024 * 128);
  EXPECT_EQ(manager.block_count(), 256);
}

TEST(ResourceManagerTest, UpdateMsgSize200K) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  manager.UpdateMsgSize(1024 * 200);
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 512);
  EXPECT_EQ(manager.real_block_size(), 1024 * 512);
  EXPECT_EQ(manager.block_count(), 128);
}

TEST(ResourceManagerTest, UpdateMsgSize1M) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  manager.UpdateMsgSize(1024 * 1024);
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 1024);
  EXPECT_EQ(manager.real_block_size(), 1024 * 1024);
  EXPECT_EQ(manager.block_count(), 64);
}

TEST(ResourceManagerTest, UpdateMsgSize2M) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  manager.UpdateMsgSize(1024 * 1024 * 2);
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 1024 * 2);
  EXPECT_EQ(manager.real_block_size(), 1024 * 1024 * 2);
  EXPECT_EQ(manager.block_count(), 32);
}

TEST(ResourceManagerTest, UpdateMsgSize4M) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  manager.UpdateMsgSize(1024 * 1024 * 4);
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 1024 * 4);
  EXPECT_EQ(manager.real_block_size(), 1024 * 1024 * 4);
  EXPECT_EQ(manager.block_count(), 32);
}

TEST(ResourceManagerTest, UpdateMsgSize5M) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  manager.UpdateMsgSize(1024 * 1024 * 5);
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 1024 * 8);
  EXPECT_EQ(manager.real_block_size(), 1024 * 1024 * 8);
  EXPECT_EQ(manager.block_count(), 16);
  EXPECT_EQ(manager.context_size(), sizeof(aimrt::plugins::sm_plugin::Context));
}

TEST(ResourceManagerTest, UpdateMsgSize8M) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  manager.UpdateMsgSize(1024 * 1024 * 8);
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 1024 * 8);
  EXPECT_EQ(manager.real_block_size(), 1024 * 1024 * 8);
  EXPECT_EQ(manager.block_count(), 16);
}

TEST(ResourceManagerTest, UpdateMsgSize16M) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  manager.UpdateMsgSize(1024 * 1024 * 16);
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 1024 * 16);
  EXPECT_EQ(manager.real_block_size(), 1024 * 1024 * 16);
  EXPECT_EQ(manager.block_count(), 16);
}

TEST(ResourceManagerTest, UpdateMsgSize32M) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  manager.UpdateMsgSize(1024 * 1024 * 32);
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 1024 * 32);
  EXPECT_EQ(manager.real_block_size(), 1024 * 1024 * 32);
  EXPECT_EQ(manager.block_count(), 8);
}

TEST(ResourceManagerTest, UpdateMsgSize64M) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  manager.UpdateMsgSize(1024 * 1024 * 64);
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 1024 * 64);
  EXPECT_EQ(manager.real_block_size(), 1024 * 1024 * 64);
  EXPECT_EQ(manager.block_count(), 8);
}

TEST(ResourceManagerTest, UpdateMsgSize128M) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  EXPECT_TRUE(manager.UpdateMsgSize(1024 * 1024 * 128));
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 1024 * 128);
  EXPECT_EQ(manager.real_block_size(), 1024 * 1024 * 128);
  EXPECT_EQ(manager.block_count(), 4);
}

TEST(ResourceManagerTest, UpdateMsgSize256M) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  EXPECT_TRUE(manager.UpdateMsgSize(1024 * 1024 * 256));
  EXPECT_EQ(manager.ceiling_msg_size(), 1024 * 1024 * 256);
  EXPECT_EQ(manager.real_block_size(), 1024 * 1024 * 256);
  EXPECT_EQ(manager.block_count(), 4);
}

TEST(ResourceManagerTest, UpdateMsgSize512M) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  EXPECT_FALSE(manager.UpdateMsgSize(1024 * 1024 * 512));
}

TEST(ResourceManagerTest, ExtraSize) {
  aimrt::plugins::sm_plugin::ResourceManager manager;
  EXPECT_EQ(manager.extra_size(), 1024);
}
