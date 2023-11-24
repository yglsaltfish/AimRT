#include "ros2_publisher_module/ros2_publisher_module.h"
#include "aimrt_module_cpp_interface/co/aimrt_context.h"
#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/schedule.h"
#include "aimrt_module_cpp_interface/co/sync_wait.h"
#include "aimrt_module_ros2_interface/channel/ros2_channel.h"

#include "yaml-cpp/yaml.h"

#include "example_ros2/msg/ros_test_msg.hpp"

namespace aimrt::examples::example_ros2_channel::ros2_publisher_module {

bool Ros2PublisherModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  try {
    // Read cfg
    const auto configurator = core_.GetConfigurator();
    if (configurator) {
      std::string file_path = std::string(configurator.GetConfigFilePath());
      if (!file_path.empty()) {
        YAML::Node cfg_node = YAML::LoadFile(file_path);
        topic_name_ = cfg_node["topic_name"].as<std::string>();
        channel_frq_ = cfg_node["channel_frq"].as<double>();
      }
    }

    // Get executor handle
    executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");
    AIMRT_CHECK_ERROR_THROW(executor_,
                            "Get executor 'work_thread_pool' failed.");

    // Register publish type
    publisher_ = core_.GetChannel().GetPublisher(topic_name_);
    AIMRT_CHECK_ERROR_THROW(publisher_,
                            "Get publisher for topic '{}' failed.", topic_name_);

    bool ret = aimrt::channel::RegisterPublishType<example_ros2::msg::RosTestMsg>(publisher_);
    AIMRT_CHECK_ERROR_THROW(ret, "Register publishType failed.");

  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool Ros2PublisherModule::Start() noexcept {
  try {
    scope_.spawn(aimrt::co::On(aimrt::co::InlineScheduler(), MainLoop()));
  } catch (const std::exception& e) {
    AIMRT_ERROR("Start failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Start succeeded.");
  return true;
}

void Ros2PublisherModule::Shutdown() noexcept {
  try {
    run_flag_ = false;
    aimrt::co::SyncWait(scope_.on_empty());
  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
    return;
  }

  AIMRT_INFO("Shutdown succeeded.");
}

// Main loop
aimrt::co::Task<void> Ros2PublisherModule::MainLoop() {
  try {
    AIMRT_INFO("Start MainLoop.");

    aimrt::co::AimRTScheduler work_thread_pool_scheduler(executor_);

    co_await aimrt::co::Schedule(work_thread_pool_scheduler);

    uint32_t count = 0;
    while (run_flag_) {
      co_await aimrt::co::ScheduleAfter(
          work_thread_pool_scheduler,
          std::chrono::milliseconds(static_cast<uint32_t>(1000 / channel_frq_)));
      count++;
      AIMRT_INFO("loop count : {} -------------------------", count);

      // publish ros event
      example_ros2::msg::RosTestMsg msg;
      msg.data = {1, 2, 3, 4};
      msg.num = count + 1000;

      AIMRT_INFO("Publish new ros event, data:\n{}",
                 example_ros2::msg::to_yaml(msg));
      aimrt::channel::Publish(publisher_, msg);
    }

    AIMRT_INFO("Exit MainLoop.");
  } catch (const std::exception& e) {
    AIMRT_ERROR("Exit MainLoop with exception, {}", e.what());
  }

  co_return;
}

}  // namespace aimrt::examples::example_ros2_channel::ros2_publisher_module
