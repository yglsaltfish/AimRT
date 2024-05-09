
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>

#include "aimrt_module_protobuf_interface/channel/protobuf_channel.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"
#include "benchmark_subscriber_module/benchmark_subscriber_module.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::cpp::protobuf_channel::benchmark_subscriber_module {

// warmup runs not to measure
static const int warmups(5);

// data structure for later evaluation
struct EvaluateData {
  bool is_finished = false;
  std::string topic_name;
  uint32_t expected_send_count;
  uint32_t real_send_count = 0;
  std::vector<float> latency_array;
  size_t rec_size = 0;
};

static std::map<uint64_t, EvaluateData*> evaluate_map;  // key: id

struct EvaluateDataOutput {
  uint32_t count = 0;
  float loss_rate = 0;
  float min_latency = std::numeric_limits<float>::max();
  float max_latency = 0;
  float avg_latency = 0;

  EvaluateDataOutput& Evaluate(const EvaluateDataOutput& data) {
    count = data.count + count;
    min_latency = std::min(data.min_latency, min_latency);
    max_latency = std::max(data.max_latency, max_latency);

    if (avg_latency != 0) {
      avg_latency = ((data.avg_latency + avg_latency) / 2);
    } else {
      avg_latency = data.avg_latency;
    }

    if (loss_rate != 0) {
      loss_rate = ((data.loss_rate + loss_rate) / 2);
    } else {
      loss_rate = data.loss_rate;
    }

    return *this;
  }

  std::string Info() {
    std::stringstream ss;
    ss << std::endl;
    ss << "+-------------+-------------+-------------+-------------+-------------+" << std::endl;
    ss << "|       count | min latency | max latency | avg latency |   loss rate |" << std::endl;
    ss << "+-------------+-------------+-------------+-------------+-------------+" << std::endl;
    ss << "| " << std::setw(11) << count << " | " << std::setw(11) << min_latency << " | " << std::setw(11)
       << max_latency << " | " << std::setw(11) << avg_latency << " | " << std::setw(11) << loss_rate << " |" << std::endl;
    ss << "+-------------+-------------+-------------+-------------+-------------+" << std::endl;
    ss << std::endl;
    return ss.str();
  }
};

static std::string Evaluate(EvaluateData* data, EvaluateDataOutput& output) {
  if (!data->is_finished) {
    return ("Evaluation [" + data->topic_name + "] failed: not finished");
  }

  std::stringstream ss;

  // remove warmup runs
  if (data->latency_array.size() >= warmups) {
    data->latency_array.erase(data->latency_array.begin(), data->latency_array.begin() + warmups);
  }

  // calculate lost packages
  uint32_t lost_packages = data->real_send_count - (data->latency_array.size() + warmups);

  // calculate loss rate
  float loss_rate = ((float)(lost_packages) / (float)(data->real_send_count)) * 100.0;

  // calculate average latency
  double latency_sum = std::accumulate(data->latency_array.begin(), data->latency_array.end(), 0.0);
  double latency_avg = latency_sum / data->latency_array.size();
  auto latency_min = *std::min_element(data->latency_array.begin(), data->latency_array.end());
  auto latency_max = *std::max_element(data->latency_array.begin(), data->latency_array.end());

  auto total_count = data->latency_array.size() + warmups;

  output.count = total_count;
  output.loss_rate = loss_rate;
  output.min_latency = latency_min / 1000000.0;
  output.max_latency = latency_max / 1000000.0;
  output.avg_latency = latency_avg / 1000000.0;

  // print result
  ss << std::endl;
  ss << "Evaluation [" << data->topic_name << "] result:" << std::endl;
  ss << "  message size       : " << data->rec_size / double(total_count) / 1024.0 << " KB" << std::endl;
  ss << "  send count         : " << data->real_send_count << std::endl;
  ss << "  receive count      : " << data->latency_array.size() + warmups << std::endl;
  ss << "  loss rate          : " << loss_rate << " %" << std::endl;
  ss << "  average latency    : " << latency_avg / 1000000.0 << " ms" << std::endl;
  ss << "  min latency        : " << latency_min / 1000000.0 << " ms" << std::endl;
  ss << "  max latency        : " << latency_max / 1000000.0 << " ms" << std::endl;
  ss << std::endl;

  return ss.str();
}

bool BenchmarkSubscriberModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  try {
    // Read cfg
    const auto configurator = core_.GetConfigurator();
    if (configurator) {
      std::string file_path = std::string(configurator.GetConfigFilePath());
      if (!file_path.empty()) {
        YAML::Node cfg_node = YAML::LoadFile(file_path);
        topic_number_ = cfg_node["topic_number"].as<uint32_t>();
        topic_name_prefix_ = cfg_node["topic_name_prefix"].as<std::string>();
      }
    }

    signal_subscriber_ = core_.GetChannelHandle().GetSubscriber("benchmark_signal");
    AIMRT_CHECK_ERROR_THROW(signal_subscriber_, "Get subscriber for topic 'benchmark_signal' failed.");
    bool ret = aimrt::channel::SubscribeCo<aimrt::protocols::example::BenchmarkSignal>(
        signal_subscriber_, std::bind(&BenchmarkSubscriberModule::BenchmarkSignalHandle, this, std::placeholders::_1));
    AIMRT_CHECK_ERROR_THROW(ret, "Subscribe failed.");

    // // 订阅事件
    for (uint32_t i = 0; i < topic_number_; i++) {
      auto topic = topic_name_prefix_ + "_" + std::to_string(i);
      auto& subscriber = subscribers_.emplace_back(core_.GetChannelHandle().GetSubscriber(topic));
      AIMRT_CHECK_ERROR_THROW(subscriber, "Get subscriber for topic '{}' failed.", topic);

      ret = aimrt::channel::SubscribeCo<aimrt::protocols::example::BenchmarkMessage>(
          subscriber, std::bind(&BenchmarkSubscriberModule::BenchmarkMessageHandle, this, std::placeholders::_1));
      AIMRT_CHECK_ERROR_THROW(ret, "Subscribe failed.");
    }
  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool BenchmarkSubscriberModule::Start() { return true; }

void BenchmarkSubscriberModule::Shutdown() {
  subscribers_.clear();

  EvaluateDataOutput all_output;

  for (auto& it : evaluate_map) {
    auto evaluate_data = it.second;
    EvaluateDataOutput output;
    auto ret = Evaluate(evaluate_data, output);
    AIMRT_INFO("{}", ret);
    all_output.Evaluate(output);
    delete evaluate_data;
  }

  AIMRT_INFO("{}", all_output.Info());
}

co::Task<void> BenchmarkSubscriberModule::BenchmarkSignalHandle(const aimrt::protocols::example::BenchmarkSignal& data) {
  AIMRT_INFO("Receive new pb event, data: {}", aimrt::Pb2CompactJson(data));

  for (auto& topic : data.topic_info()) {
    if (topic.status() == aimrt::protocols::example::BenchmarkStatus::Start) {
      auto id = topic.id();
      auto it = evaluate_map.find(id);
      if (it == evaluate_map.end()) {
        auto evaluate_data = new EvaluateData();
        evaluate_data->topic_name = topic.topic_name();
        evaluate_data->expected_send_count = topic.expected_send_count();
        evaluate_data->latency_array.reserve(topic.expected_send_count());
        evaluate_map[id] = evaluate_data;
      }
    } else if (topic.status() == aimrt::protocols::example::BenchmarkStatus::Stop) {
      auto id = topic.id();
      auto it = evaluate_map.find(id);
      if (it != evaluate_map.end()) {
        auto evaluate_data = it->second;
        evaluate_data->is_finished = true;
        evaluate_data->real_send_count = topic.real_send_count();
      }
    }
  }

  co_return;
}

co::Task<void> BenchmarkSubscriberModule::BenchmarkMessageHandle(const aimrt::protocols::example::BenchmarkMessage& data) {
  auto curr_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
  auto latency = curr_timestamp - data.timestamp();

  auto id = data.id();
  auto it = evaluate_map.find(id);
  if (it != evaluate_map.end()) {
    auto evaluate_data = it->second;
    evaluate_data->latency_array.push_back(latency);
    evaluate_data->rec_size += data.data().size();
  }

  co_return;
}

}  // namespace aimrt::examples::cpp::protobuf_channel::benchmark_subscriber_module