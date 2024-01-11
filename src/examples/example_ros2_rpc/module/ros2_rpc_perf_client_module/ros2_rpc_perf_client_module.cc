#include "ros2_rpc_perf_client_module/ros2_rpc_perf_client_module.h"

#include <algorithm>
#include <chrono>
#include <numeric>

#include "aimrt_module_cpp_interface/co/aimrt_context.h"
#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/schedule.h"
#include "aimrt_module_cpp_interface/co/sync_wait.h"

#include "yaml-cpp/yaml.h"

namespace YAML {

using namespace aimrt::examples::example_ros2_rpc::ros2_rpc_perf_client_module;

template <>
struct convert<Ros2RpcPerfClientModule::Options> {
  using Options = Ros2RpcPerfClientModule::Options;

  static Node encode(const Options& rhs) {
    Node node;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    if (node["perf_mod"].as<std::string>() == "fixed-freq")
      rhs.perf_mod = Options::PerfMod::FixedFreq;
    else
      rhs.perf_mod = Options::PerfMod::Bench;

    rhs.msg_size = node["msg_size"].as<uint32_t>();
    rhs.parallel = node["parallel"].as<uint32_t>();
    rhs.increasing_parallel = node["increasing_parallel"].as<bool>();
    rhs.freq_vec = node["freq"].as<std::vector<uint32_t>>();

    return true;
  }
};
}  // namespace YAML

namespace aimrt::examples::example_ros2_rpc::ros2_rpc_perf_client_module {

std::string generateRandomString(int minLength, int maxLength) {
  std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  int length = rand() % (maxLength - minLength + 1) + minLength;
  std::string result;

  srand(time(nullptr));  // 使用当前时间作为随机数种子

  for (int i = 0; i < length; ++i) {
    int randomIndex = rand() % chars.length();
    result += chars[randomIndex];
  }

  return result;
}

bool Ros2RpcPerfClientModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  try {
    // Read cfg
    const auto configurator = core_.GetConfigurator();
    if (configurator) {
      std::string file_path = std::string(configurator.GetConfigFilePath());
      if (!file_path.empty()) {
        YAML::Node cfg_node = YAML::LoadFile(file_path);
        options_ = cfg_node.as<Options>();
      }
    }

    msg_ = generateRandomString(options_.msg_size, options_.msg_size);

    if (options_.perf_mod == Options::PerfMod::FixedFreq) {
      time_consumption_statistics_.resize(1);
    } else {
      time_consumption_statistics_.resize(options_.parallel);
    }

    // Get rpc handle
    auto rpc_handle = core_.GetRpcHandle();
    AIMRT_CHECK_ERROR_THROW(rpc_handle, "Get rpc handle failed.");

    // Register rpc client
    bool ret = aimrt::rpc::RegisterClientFunc<
        example_ros2::srv::RosTestRpcProxy>(rpc_handle);
    AIMRT_CHECK_ERROR_THROW(ret, "Register client failed.");

    // Create rpc proxy
    proxy_ = std::make_shared<example_ros2::srv::RosTestRpcProxy>(rpc_handle);

  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool Ros2RpcPerfClientModule::Start() noexcept {
  try {
    if (options_.perf_mod == Options::PerfMod::FixedFreq) {
      scope_.spawn(co::On(co::InlineScheduler(), FixedFreqStatisticsLoop()));
    } else {
      scope_.spawn(co::On(co::InlineScheduler(), BenchStatisticsLoop()));
    }
  } catch (const std::exception& e) {
    AIMRT_ERROR("Start failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Start succeeded.");
  return true;
}

void Ros2RpcPerfClientModule::Shutdown() noexcept {
  try {
    run_flag_ = false;
    co::SyncWait(scope_.complete());
  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
    return;
  }

  AIMRT_INFO("Shutdown succeeded.");
}

co::Task<void> Ros2RpcPerfClientModule::BenchStatisticsLoop() {
  AIMRT_DEBUG("Start BenchStatisticsLoop.");

  auto client_statistics_thread_pool = core_.GetExecutorManager().GetExecutor("client_statistics_thread_pool");
  AIMRT_CHECK_ERROR_THROW(
      client_statistics_thread_pool && client_statistics_thread_pool.SupportTimerSchedule(),
      "Get executor 'client_statistics_thread_pool' failed.");

  co::AimRTScheduler client_statistics_thread_pool_scheduler(client_statistics_thread_pool);

  co_await co::Schedule(client_statistics_thread_pool_scheduler);

  co_await WaitForServiceServer();

  uint32_t loop_count = 0;
  while (run_flag_) {
    co::AsyncScope bench_scope;
    std::atomic_bool bench_run_flag = true;

    uint32_t real_parallel = options_.parallel;
    if (options_.increasing_parallel) {
      uint32_t cur_parallel = loop_count / 2 + 1;
      real_parallel = ((cur_parallel < options_.parallel) ? cur_parallel : options_.parallel);
    }

    auto start_time = std::chrono::system_clock::now();

    // 开启几个bench协程
    for (uint32_t ii = 0; ii < real_parallel; ii++) {
      bench_scope.spawn(co::On(co::InlineScheduler(), BenchLoop(ii, bench_run_flag)));
    }

    // sleep几秒
    co_await co::ScheduleAfter(
        client_statistics_thread_pool_scheduler,
        std::chrono::milliseconds(10000));

    // 等待bench协程结束
    bench_run_flag = false;
    co_await co::On(
        client_statistics_thread_pool_scheduler,
        bench_scope.complete());

    auto end_time = std::chrono::system_clock::now();
    double total_time = std::chrono::duration<double>(end_time - start_time).count() * 1e3;

    // 统计结果
    std::vector<double> gather_vec;
    for (auto& vec : time_consumption_statistics_) {
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

    AIMRT_INFO(
        R"str([{}] perf data :
parallel: {}
msg_size: {}
total_count: {}
total_time(ms): {}
qps:(/s): {}
min_time(us): {}
max_time(us): {}
avg_time(us): {}
p90_time(us): {}
p99_time(us): {}
p999_time(us): {})str",
        loop_count, real_parallel, options_.msg_size, total_count, total_time, qps,
        min_time, max_time, avg_time, p90_time, p99_time, p999_time);

    ++loop_count;
  }

  AIMRT_DEBUG("Exit BenchStatisticsLoop.");
  co_return;
}

co::Task<void> Ros2RpcPerfClientModule::FixedFreqStatisticsLoop() {
  AIMRT_DEBUG("Start FixedFreqStatisticsLoop.");

  auto client_statistics_thread_pool = core_.GetExecutorManager().GetExecutor("client_statistics_thread_pool");
  AIMRT_CHECK_ERROR_THROW(
      client_statistics_thread_pool && client_statistics_thread_pool.SupportTimerSchedule(),
      "Get executor 'client_statistics_thread_pool' failed.");

  co::AimRTScheduler client_statistics_thread_pool_scheduler(client_statistics_thread_pool);

  co_await co::Schedule(client_statistics_thread_pool_scheduler);

  co_await WaitForServiceServer();

  uint32_t loop_count = 0;
  for (auto freq : options_.freq_vec) {
    if (!run_flag_) break;

    co::AsyncScope fixedfreq_scope;
    std::atomic_bool fixedfreq_run_flag = true;

    auto start_time = std::chrono::system_clock::now();

    fixedfreq_scope.spawn(co::On(co::InlineScheduler(), FixedFreqLoop(freq, fixedfreq_run_flag)));

    // sleep几秒
    co_await co::ScheduleAfter(
        client_statistics_thread_pool_scheduler,
        std::chrono::milliseconds(10000));

    // 等待bench协程结束
    fixedfreq_run_flag = false;
    co_await co::On(
        client_statistics_thread_pool_scheduler,
        fixedfreq_scope.complete());

    auto end_time = std::chrono::system_clock::now();
    double total_time = std::chrono::duration<double>(end_time - start_time).count() * 1e3;

    // 统计结果
    std::vector<double> gather_vec;
    for (auto& vec : time_consumption_statistics_) {
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

    AIMRT_INFO(
        R"str([{}] perf data :
freq: {}
msg_size: {}
total_count: {}
total_time(ms): {}
qps:(/s): {}
min_time(us): {}
max_time(us): {}
avg_time(us): {}
p90_time(us): {}
p99_time(us): {}
p999_time(us): {})str",
        loop_count, freq, options_.msg_size, total_count, total_time, qps, min_time,
        max_time, avg_time, p90_time, p99_time, p999_time);

    ++loop_count;
  }

  AIMRT_DEBUG("Exit BenchStatisticsLoop.");
  co_return;
}

co::Task<void> Ros2RpcPerfClientModule::BenchLoop(
    int seq, std::atomic_bool& bench_run_flag) {
  AIMRT_DEBUG("Start BenchLoop {}", seq);

  std::string executor_name = "client_thread_pool_" + std::to_string(seq);
  auto client_thread_pool = core_.GetExecutorManager().GetExecutor(executor_name);
  AIMRT_CHECK_ERROR_THROW(client_thread_pool, "Get executor '{}' failed.", executor_name);

  co::AimRTScheduler client_thread_pool_scheduler(client_thread_pool);
  co_await co::Schedule(client_thread_pool_scheduler);

  example_ros2::srv::RosTestRpc_Request req;
  example_ros2::srv::RosTestRpc_Response rsp;
  req.data.resize(msg_.size());
  memcpy(req.data.data(), msg_.c_str(), msg_.size());

  std::vector<double>& time_vec_ref = time_consumption_statistics_[seq];

  while (bench_run_flag) {
    co_await co::Schedule(client_thread_pool_scheduler);

    auto task_start_time = std::chrono::system_clock::now();

    // call rpc
    auto status = co_await proxy_->RosTestRpc(req, rsp);
    co_await co::Schedule(client_thread_pool_scheduler);

    auto task_end_time = std::chrono::system_clock::now();

    AIMRT_CHECK_WARN(status, "Call rpc failed, status: {}", status.ToString());

    if (task_end_time < task_start_time) {
      AIMRT_WARN("Invalid time");
      time_vec_ref.emplace_back(0.0);  // 太小了测不出来
    } else {
      time_vec_ref.emplace_back(
          std::chrono::duration<double>(task_end_time - task_start_time).count() * 1e6);
    }
  }

  AIMRT_DEBUG("Exit BenchLoop {}", seq);

  co_return;
}

co::Task<void> Ros2RpcPerfClientModule::FixedFreqLoop(
    int32_t freq, std::atomic_bool& fixedfreq_run_flag) {
  AIMRT_DEBUG("start FixedFreqLoop");

  std::string executor_name = "client_thread_pool_0";
  auto client_thread_pool = core_.GetExecutorManager().GetExecutor(executor_name);
  AIMRT_CHECK_ERROR_THROW(client_thread_pool && client_thread_pool.SupportTimerSchedule(),
                          "Get executor '{}' failed.", executor_name);

  co::AimRTScheduler client_thread_pool_scheduler(client_thread_pool);
  co_await co::Schedule(client_thread_pool_scheduler);

  example_ros2::srv::RosTestRpc_Request req;
  example_ros2::srv::RosTestRpc_Response rsp;
  req.data.resize(msg_.size());
  memcpy(req.data.data(), msg_.c_str(), msg_.size());

  std::vector<double>& time_vec_ref = time_consumption_statistics_[0];

  while (fixedfreq_run_flag) {
    co_await co::ScheduleAt(
        client_thread_pool_scheduler,
        client_thread_pool.Now() + std::chrono::nanoseconds(1000000000 / freq));

    auto task_start_time = std::chrono::system_clock::now();

    // call rpc
    auto status = co_await proxy_->RosTestRpc(req, rsp);
    co_await co::Schedule(client_thread_pool_scheduler);

    auto task_end_time = std::chrono::system_clock::now();

    AIMRT_CHECK_WARN(status, "Call rpc failed, status: {}", status.ToString());

    if (task_end_time < task_start_time) {
      AIMRT_WARN("Invalid time");
      time_vec_ref.emplace_back(0.0);  // 太小了测不出来
    } else {
      time_vec_ref.emplace_back(
          std::chrono::duration<double>(task_end_time - task_start_time).count() * 1e6);
    }
  }

  AIMRT_DEBUG("Exit FixedFreqLoop");

  co_return;
}

co::Task<void> Ros2RpcPerfClientModule::WaitForServiceServer() {
  AIMRT_DEBUG("wait for service server...");

  std::string executor_name = "client_statistics_thread_pool";
  auto client_thread_pool = core_.GetExecutorManager().GetExecutor(executor_name);
  AIMRT_CHECK_ERROR_THROW(client_thread_pool && client_thread_pool.SupportTimerSchedule(),
                          "Get executor '{}' failed.", executor_name);

  co::AimRTScheduler client_scheduler(client_thread_pool);
  co_await co::Schedule(client_scheduler);

  example_ros2::srv::RosTestRpc_Request req;
  example_ros2::srv::RosTestRpc_Response rsp;
  req.data.resize(msg_.size());
  memcpy(req.data.data(), msg_.c_str(), msg_.size());

  while (run_flag_) {
    co_await co::ScheduleAfter(
        client_scheduler,
        std::chrono::milliseconds(1000));

    // call rpc
    auto status = co_await proxy_->RosTestRpc(req, rsp);

    if (!status.OK()) {
      AIMRT_WARN("Server is not available!!!");
    } else {
      break;
    }
  }

  AIMRT_DEBUG("Server is available!!!");

  co_return;
}

}  // namespace aimrt::examples::example_ros2_rpc::ros2_rpc_perf_client_module
