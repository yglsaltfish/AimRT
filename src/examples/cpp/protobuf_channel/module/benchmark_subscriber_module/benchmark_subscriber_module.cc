
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <limits>
#include <numeric>

#include "aimrt_module_protobuf_interface/channel/protobuf_channel.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"
#include "benchmark_subscriber_module/benchmark_subscriber_module.h"
#include "util/format.h"
#include "util/string_util.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::cpp::protobuf_channel::benchmark_subscriber_module {

bool BenchmarkSubscriberModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  try {
    // Read cfg
    auto file_path = core_.GetConfigurator().GetConfigFilePath();
    if (!file_path.empty()) {
      YAML::Node cfg_node = YAML::LoadFile(file_path.data());
      topic_number_ = cfg_node["topic_number"].as<uint32_t>();
      topic_name_prefix_ = cfg_node["topic_name_prefix"].as<std::string>();
    }

    // Subscribe
    signal_subscriber_ = core_.GetChannelHandle().GetSubscriber("benchmark_signal");
    AIMRT_CHECK_ERROR_THROW(signal_subscriber_, "Get subscriber for topic 'benchmark_signal' failed.");

    bool ret = aimrt::channel::Subscribe<aimrt::protocols::example::BenchmarkSignal>(
        signal_subscriber_, std::bind(&BenchmarkSubscriberModule::BenchmarkSignalHandle, this, std::placeholders::_1));
    AIMRT_CHECK_ERROR_THROW(ret, "Subscribe failed.");

    for (uint32_t i = 0; i < topic_number_; i++) {
      auto topic_name = topic_name_prefix_ + "_" + std::to_string(i);
      auto subscriber = core_.GetChannelHandle().GetSubscriber(topic_name);
      AIMRT_CHECK_ERROR_THROW(subscriber, "Get subscriber for topic '{}' failed.", topic_name);

      bool ret = aimrt::channel::Subscribe<aimrt::protocols::example::BenchmarkMessage>(
          subscriber,
          [this, i](const std::shared_ptr<const aimrt::protocols::example::BenchmarkMessage>& data) {
            BenchmarkMessageHandle(i, data);
          });
      AIMRT_CHECK_ERROR_THROW(ret, "Subscribe failed.");

      subscribers_.push_back(subscriber);

      topic_record_vec_.push_back(TopicRecord{.topic_name = topic_name});
    }
  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool BenchmarkSubscriberModule::Start() { return true; }

void BenchmarkSubscriberModule::Shutdown() {}

void BenchmarkSubscriberModule::BenchmarkSignalHandle(
    const std::shared_ptr<const aimrt::protocols::example::BenchmarkSignal>& data) {
  AIMRT_INFO("Receive new signal data: {}", aimrt::Pb2CompactJson(*data));

  auto topic_name = data->topic_name();
  auto finditr = std::find_if(
      topic_record_vec_.begin(), topic_record_vec_.end(),
      [&topic_name](const TopicRecord& topic_record) {
        return topic_record.topic_name == topic_name;
      });

  if (finditr == topic_record_vec_.end()) [[unlikely]] {
    AIMRT_WARN("Can not find topic name: {}", topic_name);
    return;
  }

  auto& topic_record = finditr;

  if (data->status() == aimrt::protocols::example::BenchmarkStatus::Begin) {
    topic_record->expect_send_num = data->send_num();
    topic_record->message_size = data->message_size();
    topic_record->send_frequency = data->send_frequency();

    topic_record->msg_record_vec.resize(data->send_num());

  } else if (data->status() == aimrt::protocols::example::BenchmarkStatus::End) {
    topic_record->real_send_num = data->send_num();
    topic_record->is_finished = true;

    CheckAndEvaluate();
  }
}

void BenchmarkSubscriberModule::BenchmarkMessageHandle(
    uint32_t topic_index,
    const std::shared_ptr<const aimrt::protocols::example::BenchmarkMessage>& data) {
  auto recv_timestamp = aimrt::common::util::GetCurTimestampNs();

  auto seq = data->seq();

  auto& topic_record = topic_record_vec_[topic_index];

  if (topic_record.is_finished) [[unlikely]] {
    AIMRT_WARN("Topic '{}' is end bench.", topic_record.topic_name);
    return;
  }

  auto& msg_record_vec = topic_record.msg_record_vec;

  if (seq >= msg_record_vec.size()) [[unlikely]] {
    AIMRT_WARN("Invalid seq {}, Topic '{}'.", seq, topic_record.topic_name);
    return;
  }

  auto& msg_record = msg_record_vec[seq];

  msg_record.recv = true;
  msg_record.data_size = static_cast<uint32_t>(data->data().size());
  msg_record.send_timestamp = data->timestamp();
  msg_record.recv_timestamp = recv_timestamp;
}

void BenchmarkSubscriberModule::CheckAndEvaluate() const {
  // Check all finish
  for (const auto& topic_record : topic_record_vec_) {
    if (!topic_record.is_finished)
      return;
  }

  AIMRT_INFO("End benchmark, evaluate...");

  size_t send_count = 0;
  size_t recv_count = 0;

  uint64_t min_latency = std::numeric_limits<uint64_t>::max();
  uint64_t max_latency = 0;

  uint64_t sum_latency = 0;

  for (const auto& topic_record : topic_record_vec_) {
    send_count += topic_record.real_send_num;

    const auto& msg_record_vec = topic_record.msg_record_vec;

    for (size_t seq = 0; seq < msg_record_vec.size(); ++seq) {
      auto& msg_record = msg_record_vec[seq];

      if (!msg_record.recv) continue;

      ++recv_count;

      uint64_t latency = msg_record.recv_timestamp - msg_record.send_timestamp;

      if (msg_record.recv_timestamp <= msg_record.send_timestamp) {
        AIMRT_WARN("Invalid timestamp, recv timestamp: {}, send timestamp: {}",
                   msg_record.recv_timestamp, msg_record.send_timestamp);

        latency = 0;
      }

      if (min_latency > latency) min_latency = latency;

      if (max_latency < latency) max_latency = latency;

      sum_latency += latency;
    }
  }

  uint64_t avg_latency = sum_latency / recv_count;
  double loss_rate = static_cast<double>(send_count - recv_count) / send_count;

  std::vector<std::vector<std::string>> table =
      {{"send count", "recv count", "loss rate", "min latency(us)", "max latency(us)", "avg latency(us)"},
       {std::to_string(send_count),
        std::to_string(recv_count),
        std::to_string(loss_rate),
        std::to_string(static_cast<double>(min_latency) / 1000),
        std::to_string(static_cast<double>(max_latency) / 1000),
        std::to_string(static_cast<double>(avg_latency) / 1000)}};

  AIMRT_INFO("report:\n{}", aimrt::common::util::DrawTable(table));
}

}  // namespace aimrt::examples::cpp::protobuf_channel::benchmark_subscriber_module