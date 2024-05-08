#pragma once

#include <atomic>
#include <memory>

#include "aimrt_module_cpp_interface/co/async_scope.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/module_base.h"

#include "RosTestRpc.aimrt_rpc.srv.h"

namespace aimrt::examples::cpp::ros2_rpc::benchmark_rpc_client_module {

class BenchmarkRpcClientModule : public aimrt::ModuleBase {
 public:
  struct Options {
    enum class PerfMod : uint8_t {
      Bench,
      FixedFreq
    };
    PerfMod perf_mod = PerfMod::Bench;
    uint32_t msg_size = 100;
    uint32_t parallel = 4;
    bool increasing_parallel = true;
    std::vector<uint32_t> freq_vec{24, 30, 50, 60, 120};
  };

 public:
  BenchmarkRpcClientModule() = default;
  ~BenchmarkRpcClientModule() override = default;

  ModuleInfo Info() const override {
    return ModuleInfo{.name = "BenchmarkRpcClientModule"};
  }

  bool Initialize(aimrt::CoreRef core) override;

  bool Start() override;

  void Shutdown() override;

 private:
  auto GetLogger() { return core_.GetLogger(); }

  co::Task<void> BenchStatisticsLoop();
  co::Task<void> BenchLoop(int seq, std::atomic_bool& bench_run_flag);

  co::Task<void> FixedFreqStatisticsLoop();
  co::Task<void> FixedFreqLoop(int32_t freq, std::atomic_bool& bench_run_flag);

  co::Task<void> WaitForServiceServer();

 private:
  Options options_;

  aimrt::CoreRef core_;
  co::AsyncScope scope_;
  std::atomic_bool run_flag_ = true;

  std::shared_ptr<example_ros2::srv::RosTestRpcProxy> proxy_;

  std::string msg_;
  std::vector<std::vector<double> > time_consumption_statistics_;
};

}  // namespace aimrt::examples::cpp::ros2_rpc::benchmark_rpc_client_module
