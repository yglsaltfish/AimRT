#include "ros2_subscriber_module/ros2_subscriber_module.h"
#include "aimrt_module_ros2_interface/channel/ros2_channel.h"

#include "yaml-cpp/yaml.h"

#include "example_ros2/msg/ros_test_msg.hpp"

namespace aimrt::examples::example_ros2_channel::ros2_subscriber_module {

bool Ros2SubscriberModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  try {
    // Read cfg
    const auto configurator = core_.GetConfigurator();
    if (configurator) {
      std::string file_path = std::string(configurator.GetConfigFilePath());
      if (!file_path.empty()) {
        YAML::Node cfg_node = YAML::LoadFile(file_path);
        topic_name_ = cfg_node["topic_name"].as<std::string>();
      }
    }

    // 订阅事件
    subscriber_ = core_.GetChannelHandle().GetSubscriber(topic_name_);
    AIMRT_CHECK_ERROR_THROW(subscriber_,
                            "Get subscriber for topic '{}' failed.", topic_name_);

    bool ret = aimrt::channel::Subscribe<example_ros2::msg::RosTestMsg>(
        subscriber_,
        [this](const std::shared_ptr<const example_ros2::msg::RosTestMsg>& msg) {
          AIMRT_INFO("Receive new ros event, data:\n{}", example_ros2::msg::to_yaml(*msg));
          return;
        });
    AIMRT_CHECK_ERROR_THROW(ret, "Subscribe failed.");

  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool Ros2SubscriberModule::Start() noexcept { return true; }

void Ros2SubscriberModule::Shutdown() noexcept {}

}  // namespace aimrt::examples::example_ros2_channel::ros2_subscriber_module
