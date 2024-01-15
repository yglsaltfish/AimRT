
#include "benchmark_publisher_module/benchmark_publisher_module.h"
#include "aimrt_module_cpp_interface/co/aimrt_context.h"
#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/schedule.h"
#include "aimrt_module_cpp_interface/co/sync_wait.h"
#include "aimrt_module_protobuf_interface/channel/protobuf_channel.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"

#include "yaml-cpp/yaml.h"

#include "benchmark.pb.h"

namespace aimrt::examples::example_normal_channel::benchmark_publisher_module {

static std::mutex signal_mutex;
static aimrt::protocols::example::BenchmarkSignal signal;

bool BenchmarkPublisherModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  try {
    // Read cfg
    const auto configurator = core_.GetConfigurator();
    if (configurator) {
      std::string file_path = std::string(configurator.GetConfigFilePath());
      if (!file_path.empty()) {
        YAML::Node cfg_node = YAML::LoadFile(file_path);
        channel_frq_ = cfg_node["channel_frq"].as<double>();
        msg_size_ = cfg_node["msg_size"].as<uint32_t>();
        msg_count_ = cfg_node["msg_count"].as<uint32_t>();
        topic_number_ = cfg_node["topic_number"].as<uint32_t>();
        topic_name_prefix_ = cfg_node["topic_name_prefix"].as<std::string>();
      }
    }

    // Get executor handle
    executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");
    AIMRT_CHECK_ERROR_THROW(executor_, "Get executor 'work_thread_pool' failed.");

    // Register publish type
    signal_publisher_ = core_.GetChannelHandle().GetPublisher("benchmark_signal");
    AIMRT_CHECK_ERROR_THROW(signal_publisher_, "Get publisher for topic 'benchmark_signal' failed.");
    aimrt::channel::RegisterPublishType<aimrt::protocols::example::BenchmarkSignal>(signal_publisher_);

    for (uint32_t i = 0; i < topic_number_; i++) {
      auto topic = topic_name_prefix_ + "_" + std::to_string(i);
      publishers_.push_back(core_.GetChannelHandle().GetPublisher(topic));
      auto& publisher = publishers_[i];
      AIMRT_CHECK_ERROR_THROW(publisher, "Get publisher for topic '{}' failed.", topic);
      bool ret = aimrt::channel::RegisterPublishType<aimrt::protocols::example::BenchmarkMessage>(publisher);
      AIMRT_CHECK_ERROR_THROW(ret, "Register publish type failed.");

      std::lock_guard<std::mutex> lock(signal_mutex);
      auto topic_info = signal.add_topic_info();
      topic_info->set_topic_name(topic);
      topic_info->set_message_size(msg_size_);
      topic_info->set_send_frequency(channel_frq_);
      topic_info->set_expected_send_count(msg_count_);
      topic_info->set_id(std::hash<std::string>{}(topic));
    }

  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool BenchmarkPublisherModule::Start() noexcept {
  try {
    scope_.spawn(co::On(co::InlineScheduler(), MainLoop()));
  } catch (const std::exception& e) {
    AIMRT_ERROR("Start failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Start succeeded.");
  return true;
}

void BenchmarkPublisherModule::Shutdown() noexcept {
  try {
    run_flag_ = false;

    for (auto& future : futures_) {
      future.wait();
    }

    co::SyncWait(scope_.complete());

  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
    return;
  }

  AIMRT_INFO("Shutdown succeeded.");
}

// Main loop
co::Task<void> BenchmarkPublisherModule::MainLoop() {
  try {
    AIMRT_INFO("Start MainLoop.");

    for (uint32_t i = 0; i < topic_number_; i++) {
      // 在executor上周期性发布事件
      auto task = std::make_shared<std::packaged_task<void()>>([this, i]() {
        auto topic_name = topic_name_prefix_ + "_" + std::to_string(i);
        {
          std::lock_guard<std::mutex> lock(signal_mutex);
          auto topic_info = signal.mutable_topic_info(i);
          topic_info->set_status(aimrt::protocols::example::BenchmarkStatus::Start);
          aimrt::channel::Publish(signal_publisher_, signal);
          AIMRT_INFO("Publish benchmark signal, data: {}", aimrt::Pb2CompactJson(signal));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::string data(msg_size_, 'a');
        uint32_t sleep_us = static_cast<uint32_t>(1000000 / channel_frq_);

        auto& publisher = publishers_[i];
        uint32_t send_count = 0;

        aimrt::protocols::example::BenchmarkMessage msg;
        msg.set_data(data);
        msg.set_topic_name(topic_name);
        msg.set_frequency(channel_frq_);
        msg.set_id(std::hash<std::string>{}(topic_name));

        for (send_count = 0; send_count < msg_count_; send_count++) {
          msg.set_seq(send_count);
          uint64_t timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count();
          msg.set_timestamp(timestamp);
          if (!run_flag_) break;
          aimrt::channel::Publish(publisher, msg);
          std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
        }

        // 发布结束通知信号
        {
          std::lock_guard<std::mutex> lock(signal_mutex);
          auto topic_info = signal.mutable_topic_info(i);
          topic_info->set_status(aimrt::protocols::example::BenchmarkStatus::Stop);
          topic_info->set_real_send_count(send_count);
          aimrt::channel::Publish(signal_publisher_, signal);
          AIMRT_INFO("Topic '{}' publish {} messages.", topic_name, send_count);
        }
      });

      futures_.push_back(std::move(task->get_future()));

      executor_.Execute([this, i, task]() {
        try {
          (*task)();
        } catch (const std::exception& e) {
          AIMRT_ERROR("Publish task failed, {}", e.what());
        }
      });
    }
  } catch (const std::exception& e) {
    AIMRT_ERROR("Exit MainLoop with exception, {}", e.what());
  }

  co_return;
}

}  // namespace aimrt::examples::example_normal_channel::benchmark_publisher_module