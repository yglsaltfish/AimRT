#include "normal_publisher_module/normal_publisher_module.h"
#include "aimrt_module_cpp_interface/co/aimrt_context.h"
#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/schedule.h"
#include "aimrt_module_cpp_interface/co/sync_wait.h"
#include "aimrt_module_protobuf_interface/channel/protobuf_channel.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::example_normal_channel::normal_publisher_module {

bool NormalPublisherModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  try {
    // Read cfg
    const auto configurator = core_.GetConfigurator();
    if (configurator) {
      YAML::Node cfg_node =
          YAML::LoadFile(std::string(configurator.GetConfigFilePath()));
      publish_topic_name_ = cfg_node["publish_topic_name"].as<std::string>();
      subscribe_topic_name_ = cfg_node["subscribe_topic_name"].as<std::string>();
      channel_frq_ = cfg_node["channel_frq"].as<double>();
    }

    // Get executor handle
    executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");
    AIMRT_CHECK_ERROR_THROW(executor_ && executor_.SupportTimerSchedule(),
                            "Get executor 'work_thread_pool' failed.");

    // Register publish type
    publisher_ = core_.GetChannelHandle().GetPublisher(publish_topic_name_);
    AIMRT_CHECK_ERROR_THROW(publisher_, "Get publisher for topic '{}' failed.", publish_topic_name_);

    bool ret = aimrt::channel::RegisterPublishType<
        aimrt::protocols::example::ExampleEventMsg>(publisher_);
    AIMRT_CHECK_ERROR_THROW(ret, "Register publish type failed.");

    // 订阅事件
    subscriber_ = core_.GetChannelHandle().GetSubscriber(subscribe_topic_name_);
    AIMRT_CHECK_ERROR_THROW(
        subscriber_, "Get subscriber for topic '{}' failed.", subscribe_topic_name_);

    ret = aimrt::channel::SubscribeCo<aimrt::protocols::example::ExampleEventMsg>(
        subscriber_, std::bind(&NormalPublisherModule::EventHandle, this, std::placeholders::_1));
    AIMRT_CHECK_ERROR_THROW(ret, "Subscribe failed.");

  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool NormalPublisherModule::Start() noexcept {
  try {
    scope_.spawn(co::On(co::InlineScheduler(), MainLoop()));
  } catch (const std::exception& e) {
    AIMRT_ERROR("Start failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Start succeeded.");
  return true;
}

void NormalPublisherModule::Shutdown() noexcept {
  try {
    run_flag_ = false;
    co::SyncWait(scope_.complete());
  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
    return;
  }

  AIMRT_INFO("Shutdown succeeded.");
}

// Main loop
co::Task<void> NormalPublisherModule::MainLoop() {
  try {
    AIMRT_INFO("Start MainLoop.");

    co::AimRTScheduler work_thread_pool_scheduler(executor_);

    co_await co::Schedule(work_thread_pool_scheduler);

    uint32_t count = 0;
    while (run_flag_) {
      co_await co::ScheduleAfter(
          work_thread_pool_scheduler,
          std::chrono::microseconds(static_cast<uint32_t>(1000000 / channel_frq_)));
      count++;
      AIMRT_INFO("Loop count : {} -------------------------", count);

      // publish event
      aimrt::protocols::example::ExampleEventMsg msg;
      msg.set_msg("count: " + std::to_string(count));
      msg.set_num(count);
      AIMRT_INFO("Publish new pb event, data: {}",
                 aimrt::Pb2CompactJson(msg));
      aimrt::channel::Publish(publisher_, msg);
    }

    AIMRT_INFO("Exit MainLoop.");
  } catch (const std::exception& e) {
    AIMRT_ERROR("Exit MainLoop with exception, {}", e.what());
  }

  co_return;
}

co::Task<void> NormalPublisherModule::EventHandle(
    const aimrt::protocols::example::ExampleEventMsg& data) {
  AIMRT_INFO("Get new pb event, data: {}", aimrt::Pb2CompactJson(data));

  co_return;
}

}  // namespace aimrt::examples::example_normal_channel::normal_publisher_module
