#include "core/channel/channel_registry.h"
#include <gtest/gtest.h>

namespace aimrt::runtime::core::channel {
class ChannelRegistryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    publish_type_wrapper_test_ptr_->msg_type = "publish_msg_type_test";
    publish_type_wrapper_test_ptr_->pkg_path = "publish_pkg_path_test";
    publish_type_wrapper_test_ptr_->module_name = "publish_module_name_test";
    publish_type_wrapper_test_ptr_->topic_name = "publish_topic_name_test";

    subscribe_wrapper_test_ptr_->msg_type = "subscribe_msg_type_test";
    subscribe_wrapper_test_ptr_->pkg_path = "subscribe_pkg_path_test";
    subscribe_wrapper_test_ptr_->module_name = "subscribe_module_name_test";
    subscribe_wrapper_test_ptr_->topic_name = "subscribe_topic_name_test";
  }
  // 测试用例结束后清理工作
  void TearDown() override {
    ;
  }

  ChannelRegistry channel_registry_test_;
  std::unique_ptr<PublishTypeWrapper> publish_type_wrapper_test_ptr_ = std::make_unique<PublishTypeWrapper>();
  std::unique_ptr<SubscribeWrapper> subscribe_wrapper_test_ptr_ = std::make_unique<SubscribeWrapper>();
};

// 测试RegisterPublishType 、 GetPubTopicIndexMap 和GetPublishTypeWrapperMap()
TEST_F(ChannelRegistryTest, RegisterPublishType_GetPubTopicIndexMap) {
  EXPECT_EQ(channel_registry_test_.GetPubTopicIndexMap().size(), 0);
  EXPECT_EQ(channel_registry_test_.RegisterPublishType(std::move(publish_type_wrapper_test_ptr_)), true);
  auto pub_topic_index_map_test_ = channel_registry_test_.GetPubTopicIndexMap();
  EXPECT_EQ(pub_topic_index_map_test_.size(), 1);
  EXPECT_TRUE(pub_topic_index_map_test_.find("publish_topic_name_test") != pub_topic_index_map_test_.end());
  auto publisher_type_wrapper_test_ = channel_registry_test_.GetPublishTypeWrapper(
      "publish_pkg_path_test",
      "publish_module_name_test",
      "publish_topic_name_test",
      "publish_msg_type_test");
  EXPECT_EQ(publisher_type_wrapper_test_->module_name, "publish_module_name_test");

  EXPECT_EQ(channel_registry_test_.GetPublishTypeWrapperMap().size(), 1);
}

// 测试Subscribe 、 GetSubscribeWrapperMap 和GetSubTopicIndexMap()
TEST_F(ChannelRegistryTest, Subscribe_GetSubscribeWrapperMap) {
  EXPECT_EQ(channel_registry_test_.GetSubTopicIndexMap().size(), 0);
  EXPECT_EQ(channel_registry_test_.Subscribe(std::move(subscribe_wrapper_test_ptr_)), true);
  auto sub_topic_index_map_test_ = channel_registry_test_.GetSubTopicIndexMap();
  EXPECT_EQ(sub_topic_index_map_test_.size(), 1);
  EXPECT_TRUE(sub_topic_index_map_test_.find("subscribe_topic_name_test") != sub_topic_index_map_test_.end());
  EXPECT_EQ(channel_registry_test_.GetSubscribeWrapperMap().size(), 1);
}
}  // namespace aimrt::runtime::core::channel