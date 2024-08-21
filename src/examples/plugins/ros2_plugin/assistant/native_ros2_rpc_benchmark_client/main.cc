// Copyright (c) 2023, AgiBot Inc.
// All rights reserved

#include <chrono>
#include <cinttypes>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "example_ros2/srv/ros_test_rpc.hpp"
#include "rclcpp/rclcpp.hpp"

using RosTestRpc = example_ros2::srv::RosTestRpc;
using namespace std::chrono_literals;

std::string generateRandomString(int minLength, int maxLength) {
  std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  int length = rand() % (maxLength - minLength + 1) + minLength;
  std::string result;

  srand(time(nullptr));

  for (int i = 0; i < length; ++i) {
    int randomIndex = rand() % chars.length();
    result += chars[randomIndex];
  }

  return result;
}

struct Options {
  enum class PerfMod : uint8_t {
    Bench,
    FixedFreq
  };
  PerfMod perf_mod = PerfMod::Bench;
  uint32_t msg_size = 1048576;
  uint32_t parallel = 8;
  bool increasing_parallel = true;
  std::vector<uint32_t> freq_vec{24, 30, 50, 60, 120};
};

class RosTestRpcPerfClient : public rclcpp::Node {
 public:
  explicit RosTestRpcPerfClient(
      const std::string &node_name, int seq, const Options &options,
      std::atomic_bool &run_flag, std::vector<double> &time_vec,
      const rclcpp::NodeOptions &node_options = rclcpp::NodeOptions{})
      : Node(node_name, node_options),
        seq_(seq),
        options_(options),
        run_flag_(run_flag),
        time_vec_(time_vec) {
    thread_name = node_name;
    // set QoS
    rclcpp::QoS qos(rclcpp::KeepLast(1000));
    qos.reliable();
    qos.lifespan(std::chrono::seconds(30));
    // rclcpp::QoS qos(rclcpp::KeepAll());

    client_ = create_client<RosTestRpc>(
        "/example_ros2/srv/RosTestRpc",
        qos.get_rmw_qos_profile());

    request_ = std::make_shared<RosTestRpc::Request>();
    auto msg = generateRandomString(options_.msg_size, options_.msg_size);
    request_->data.resize(msg.size());
    memcpy(request_->data.data(), msg.c_str(), msg.size());
  }

  bool wait_for_service_server() {
    while (!client_->wait_for_service(std::chrono::seconds(1))) {
      if (!rclcpp::ok()) {
        RCLCPP_ERROR(this->get_logger(),
                     "client interrupted while waiting for service to appear.");
        return false;
      }
      RCLCPP_INFO(this->get_logger(), "waiting for service to appear...");
    }
    return true;
  }

  void bench_queue_async_request(std::promise<void> *ready_promise) {
    if (!run_flag_.load()) {
      ready_promise->set_value();
      return;
    }

    using ServiceResponseFuture = rclcpp::Client<RosTestRpc>::SharedFuture;

    auto task_start_time = std::chrono::steady_clock::now();

    auto result = client_->async_send_request(
        request_,
        [this, task_start_time, ready_promise](ServiceResponseFuture future) {
          auto request_response = future.get();
          if (request_response->code != 123) {
            RCLCPP_WARN(this->get_logger(), "err with rpc");
            ready_promise->set_value();
            return;
          }

          auto task_end_time = std::chrono::steady_clock::now();

          if (task_end_time < task_start_time) {
            time_vec_.emplace_back(0.0);  // too small to measure
          } else {
            time_vec_.emplace_back(
                std::chrono::duration<double>(task_end_time - task_start_time).count() * 1e6);
          }

          bench_queue_async_request(ready_promise);
        });
  }

  void fixed_freq_queue_async_request(std::promise<void> *ready_promise,
                                      int freq_vec) {
    using ServiceResponseFuture = rclcpp::Client<RosTestRpc>::SharedFuture;

    auto task_start_time = std::chrono::steady_clock::now();
    auto result = client_->async_send_request(
        request_,
        [this, task_start_time, ready_promise, freq_vec](ServiceResponseFuture future) {
          auto request_response = future.get();
          if (request_response->code != 123) {
            RCLCPP_WARN(this->get_logger(), "err with rpc");
            ready_promise->set_value();
            return;
          }

          auto task_end_time = std::chrono::steady_clock::now();

          if (task_end_time < task_start_time) {
            time_vec_.emplace_back(0.0);  // too small to measure
          } else {
            time_vec_.emplace_back(
                std::chrono::duration<double>(task_end_time - task_start_time).count() * 1e6);
          }
        });
  }

 public:
  rclcpp::Client<RosTestRpc>::SharedPtr client_;

  int seq_ = 0;
  Options options_;
  std::atomic_bool &run_flag_;
  std::vector<double> &time_vec_;
  std::string thread_name;

  std::shared_ptr<RosTestRpc::Request> request_;
};

void FixedFreqStatisticsLoop(const Options &options) {
  std::atomic_bool run_flag = true;
  std::vector<std::vector<double>> time_consumption_statistics(1);

  std::shared_ptr<RosTestRpcPerfClient> node =
      std::make_shared<RosTestRpcPerfClient>(
          "native_ros2_rpc_benchmark_client_" + std::to_string(0),
          0, options, run_flag, time_consumption_statistics[0]);
  const auto &logger = node->get_logger();

  RCLCPP_INFO(logger, "waiting for service to appear...");
  if (!node->wait_for_service_server()) {
    RCLCPP_INFO(logger, "Server is not available!!!");
    return;
  }
  RCLCPP_INFO(logger, "Server is available!!!");

  std::thread ros2_thread([node]() {
#ifndef _WIN32
    pthread_setname_np(pthread_self(), node->thread_name.c_str());
#endif
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();
    rclcpp::shutdown();
    fprintf(stderr, "exit %d\n", node->seq_);
  });

  std::this_thread::sleep_for(1000ms);

  uint32_t loop_count = 0;
  while (rclcpp::ok()) {
    int index = 0;
    if (loop_count >= options.freq_vec.size()) {
      index = options.freq_vec.size() - 1;
    } else {
      index = loop_count;
    }
    uint32_t freq_vec = options.freq_vec[index];

    std::promise<void> promise_;

    auto start_time = std::chrono::steady_clock::now();

    run_flag.store(true);

    std::thread send_thread(
        [&run_flag, &node, &promise_, freq_vec]() {
          while (run_flag.load()) {
            node->fixed_freq_queue_async_request(&promise_, freq_vec);
            std::this_thread::sleep_for(std::chrono::nanoseconds(1000000000 / freq_vec));
          }
          promise_.set_value();
          return;
        });

    std::this_thread::sleep_for(5s);
    run_flag.store(false);

    promise_.get_future().wait();

    auto end_time = std::chrono::steady_clock::now();
    double total_time = std::chrono::duration<double>(end_time - start_time).count() * 1e3;

    // 统计结果
    std::vector<double> gather_vec;
    for (auto &vec : time_consumption_statistics) {
      gather_vec.insert(gather_vec.begin(), vec.begin(), vec.end());
      vec.clear();
    }

    std::sort(gather_vec.begin(), gather_vec.end());
    size_t total_count = gather_vec.size();
    double qps = total_count * 1000 / total_time;
    double min_time = gather_vec[0];
    double max_time = gather_vec[gather_vec.size() - 1];
    double avg_time = std::accumulate(gather_vec.begin(), gather_vec.end(), double(0.0)) / total_count;
    double p90_time = gather_vec[total_count * 0.9];
    double p99_time = gather_vec[total_count * 0.99];
    double p999_time = gather_vec[total_count * 0.999];

    RCLCPP_INFO(node->get_logger(),
                R"str([%u] perf data :
freq_vec: %u
total_count: %lu
total_time(ms): %f
msg size: %d
qps:(/s): %f
min_time(us): %f
max_time(us): %f
avg_time(us): %f
p90_time(us): %f
p99_time(us): %f
p999_time(us): %f)str",
                loop_count, freq_vec, total_count, total_time, options.msg_size,
                qps, min_time, max_time, avg_time, p90_time, p99_time,
                p999_time);

    ++loop_count;
    send_thread.join();
  }

  ros2_thread.join();
}

void BenchStatisticsLoop(const Options &options) {
  std::atomic_bool run_flag = true;
  std::vector<std::vector<double>> time_consumption_statistics(options.parallel);

  std::vector<std::shared_ptr<RosTestRpcPerfClient>> client_vec;
  for (uint32_t ii = 0; ii < options.parallel; ++ii) {
    auto node = std::make_shared<RosTestRpcPerfClient>(
        "native_ros2_rpc_benchmark_client_" + std::to_string(ii),
        ii, options, run_flag, time_consumption_statistics[ii]);
    const auto &logger = node->get_logger();

    RCLCPP_INFO(logger, "waiting for service to appear...");
    if (!node->wait_for_service_server()) {
      RCLCPP_INFO(logger, "Server is not available!!!");
      return;
    }
    RCLCPP_INFO(logger, "Server is available!!!");

    client_vec.emplace_back(node);
  }

  std::list<std::thread> thread_list;
  for (uint32_t ii = 0; ii < options.parallel; ++ii) {
    thread_list.emplace_back(
        [node = client_vec[ii]]() {
#ifndef _WIN32
          pthread_setname_np(pthread_self(), node->thread_name.c_str());
#endif
          rclcpp::executors::SingleThreadedExecutor executor;
          executor.add_node(node);
          executor.spin();
          rclcpp::shutdown();
          fprintf(stderr, "exit thread %d\n", node->seq_);
        });
  }

  std::this_thread::sleep_for(1000ms);

  uint32_t loop_count = 0;
  while (rclcpp::ok()) {
    uint32_t real_parallel = options.parallel;
    if (options.increasing_parallel) {
      uint32_t cur_parallel = loop_count + 1;
      real_parallel = ((cur_parallel < options.parallel) ? cur_parallel : options.parallel);
    }
    std::vector<std::promise<void>> promise_vec(real_parallel);

    auto start_time = std::chrono::steady_clock::now();

    run_flag.store(true);
    for (size_t ii = 0; ii < real_parallel; ++ii) {
      client_vec[ii]->bench_queue_async_request(&(promise_vec[ii]));
    }

    std::this_thread::sleep_for(10s);
    run_flag.store(false);

    for (size_t ii = 0; ii < real_parallel; ++ii) {
      promise_vec[ii].get_future().wait();
    }
    auto end_time = std::chrono::steady_clock::now();
    double total_time = std::chrono::duration<double>(end_time - start_time).count() * 1e3;

    // statistical
    std::vector<double> gather_vec;
    for (auto &vec : time_consumption_statistics) {
      gather_vec.insert(gather_vec.begin(), vec.begin(), vec.end());
      vec.clear();
    }

    std::sort(gather_vec.begin(), gather_vec.end());
    size_t total_count = gather_vec.size();
    double qps = total_count * 1000 / total_time;
    double min_time = gather_vec[0];
    double max_time = gather_vec[gather_vec.size() - 1];
    double avg_time = std::accumulate(gather_vec.begin(), gather_vec.end(), double(0.0)) / total_count;
    double p90_time = gather_vec[total_count * 0.9];
    double p99_time = gather_vec[total_count * 0.99];
    double p999_time = gather_vec[total_count * 0.999];

    RCLCPP_INFO(client_vec[0]->get_logger(),
                R"str([%u] perf data :
parallel: %u
total_count: %lu
total_time(ms): %f
msg size: %d
qps:(/s): %f
min_time(us): %f
max_time(us): %f
avg_time(us): %f
p90_time(us): %f
p99_time(us): %f
p999_time(us): %f)str",
                loop_count, real_parallel, total_count, total_time,
                options.msg_size, qps, min_time, max_time, avg_time, p90_time,
                p99_time, p999_time);

    ++loop_count;
  }

  for (auto &ros2_thread : thread_list) {
    ros2_thread.join();
  }
}

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);

  Options options{
      .perf_mod = Options::PerfMod::Bench,
      .msg_size = 1024,
      .parallel = 5,
      .increasing_parallel = true,
      .freq_vec = {24, 30, 50, 60, 120}};

  if (options.perf_mod == Options::PerfMod::Bench) {
    BenchStatisticsLoop(options);
  } else if (options.perf_mod == Options::PerfMod::FixedFreq) {
    FixedFreqStatisticsLoop(options);
  }

  return 0;
}
