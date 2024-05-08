#include "normal_subscriber_module/normal_subscriber_module.h"
#include "aimrt_module_ros2_interface/channel/ros2_channel.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::cpp::ros2_channel::normal_subscriber_module {

bool NormalSubscriberModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  try {
    // Read cfg
    const auto configurator = core_.GetConfigurator();
    if (configurator) {
      YAML::Node cfg_node = YAML::LoadFile(std::string(configurator.GetConfigFilePath()));
      topic_name_ = cfg_node["topic_name"].as<std::string>();
    }

    // 订阅事件
    subscriber_ = core_.GetChannelHandle().GetSubscriber(topic_name_);
    AIMRT_CHECK_ERROR_THROW(subscriber_, "Get subscriber for topic '{}' failed.", topic_name_);

    bool ret = aimrt::channel::SubscribeCo<example_ros2::msg::RosTestMsg>(
        subscriber_,
        std::bind(&NormalSubscriberModule::EventHandle, this, std::placeholders::_1));
    AIMRT_CHECK_ERROR_THROW(ret, "Subscribe failed.");

  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool NormalSubscriberModule::Start() { return true; }

void NormalSubscriberModule::Shutdown() {}

co::Task<void> NormalSubscriberModule::EventHandle(const example_ros2::msg::RosTestMsg& data) {
  AIMRT_INFO("Receive new ros event, data:\n{}", example_ros2::msg::to_yaml(data));

  co_return;
}

}  // namespace aimrt::examples::cpp::ros2_channel::normal_subscriber_module
