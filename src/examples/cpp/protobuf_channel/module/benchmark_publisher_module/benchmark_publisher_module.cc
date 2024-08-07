
#include "benchmark_publisher_module/benchmark_publisher_module.h"
#include "aimrt_module_protobuf_interface/channel/protobuf_channel.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"
#include "util/time_util.h"

#include "yaml-cpp/yaml.h"

#include "benchmark.pb.h"

namespace aimrt::examples::cpp::protobuf_channel::benchmark_publisher_module {

bool BenchmarkPublisherModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  try {
    // Read cfg
    auto file_path = core_.GetConfigurator().GetConfigFilePath();
    if (!file_path.empty()) {
      YAML::Node cfg_node = YAML::LoadFile(file_path.data());
      channel_frq_ = cfg_node["channel_frq"].as<uint32_t>();
      msg_size_ = cfg_node["msg_size"].as<uint32_t>();
      msg_count_ = cfg_node["msg_count"].as<uint32_t>();
      topic_number_ = cfg_node["topic_number"].as<uint32_t>();
      topic_name_prefix_ = cfg_node["topic_name_prefix"].as<std::string>();
    }

    // Get executor handle
    executor_ = core_.GetExecutorManager().GetExecutor("publish_thread_pool");
    AIMRT_CHECK_ERROR_THROW(executor_, "Get executor 'publish_thread_pool' failed.");

    // Register publish type
    signal_publisher_ = core_.GetChannelHandle().GetPublisher("benchmark_signal");
    AIMRT_CHECK_ERROR_THROW(signal_publisher_, "Get publisher for topic 'benchmark_signal' failed.");

    bool ret = aimrt::channel::RegisterPublishType<aimrt::protocols::example::BenchmarkSignal>(signal_publisher_);
    AIMRT_CHECK_ERROR_THROW(ret, "Register publish type failed.");

    for (uint32_t i = 0; i < topic_number_; i++) {
      auto topic_name = topic_name_prefix_ + "_" + std::to_string(i);
      auto publisher = core_.GetChannelHandle().GetPublisher(topic_name);
      AIMRT_CHECK_ERROR_THROW(publisher, "Get publisher for topic '{}' failed.", topic_name);

      bool ret = aimrt::channel::RegisterPublishType<aimrt::protocols::example::BenchmarkMessage>(publisher);
      AIMRT_CHECK_ERROR_THROW(ret, "Register publish type failed.");

      publishers_.push_back(publisher);
    }

  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool BenchmarkPublisherModule::Start() {
  try {
    executor_.Execute(std::bind(&BenchmarkPublisherModule::MainLoop, this));
  } catch (const std::exception& e) {
    AIMRT_ERROR("Start failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Start succeeded.");
  return true;
}

void BenchmarkPublisherModule::Shutdown() {
  try {
    run_flag_ = false;

    for (auto& future : futures_) {
      future.wait();
    }

    stop_sig_.get_future().wait();

  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
    return;
  }

  AIMRT_INFO("Shutdown succeeded.");
}

// Main loop
void BenchmarkPublisherModule::MainLoop() {
  try {
    AIMRT_INFO("Start MainLoop.");

    for (uint32_t i = 0; i < topic_number_; i++) {
      auto task = std::make_shared<std::packaged_task<void()>>([this, i]() {
        auto topic_name = topic_name_prefix_ + "_" + std::to_string(i);

        // publish begin signal
        {
          aimrt::protocols::example::BenchmarkSignal begin_signal;
          begin_signal.set_status(aimrt::protocols::example::BenchmarkStatus::Begin);
          begin_signal.set_topic_name(topic_name);
          begin_signal.set_send_num(msg_count_);
          begin_signal.set_message_size(msg_size_);
          begin_signal.set_send_frequency(channel_frq_);

          AIMRT_INFO("Publish benchmark start signal, data: {}", aimrt::Pb2CompactJson(begin_signal));
          aimrt::channel::Publish(signal_publisher_, begin_signal);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        // publish msg
        aimrt::protocols::example::BenchmarkMessage msg;
        msg.set_data(std::string(msg_size_, 'a'));

        auto& publisher = publishers_[i];
        uint32_t send_count = 0;

        uint32_t sleep_us = static_cast<uint32_t>(1000000 / channel_frq_);
        auto cur_tp = std::chrono::system_clock::now();

        for (; send_count < msg_count_; ++send_count) {
          if (!run_flag_) [[unlikely]]
            break;

          msg.set_seq(send_count);
          msg.set_timestamp(aimrt::common::util::GetCurTimestampNs());

          aimrt::channel::Publish(publisher, msg);

          cur_tp += std::chrono::microseconds(sleep_us);
          std::this_thread::sleep_until(cur_tp);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        // publish begin signal
        {
          aimrt::protocols::example::BenchmarkSignal end_signal;
          end_signal.set_status(aimrt::protocols::example::BenchmarkStatus::End);
          end_signal.set_topic_name(topic_name);
          end_signal.set_send_num(send_count);

          AIMRT_INFO("Publish benchmark start signal, data: {}", aimrt::Pb2CompactJson(end_signal));
          aimrt::channel::Publish(signal_publisher_, end_signal);
        }
      });

      futures_.push_back(std::move(task->get_future()));

      executor_.Execute([this, task]() {
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

  stop_sig_.set_value();
}

}  // namespace aimrt::examples::cpp::protobuf_channel::benchmark_publisher_module