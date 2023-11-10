#pragma once

#include <atomic>
#include <memory>

#include "aimrt_module_cpp_interface/co/async_scope.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/module_base.h"

#include "RosTestRpc.aimrt_rpc.srv.h"

namespace aimrt::examples::example_ros2_rpc::ros2_rpc_perf_client_module {

class Ros2RpcPerfClientModule : public aimrt::ModuleBase {
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
  Ros2RpcPerfClientModule() = default;
  ~Ros2RpcPerfClientModule() override = default;

  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "Ros2RpcPerfClientModule"};
  }

  bool Initialize(aimrt::CoreRef core) noexcept override;

  bool Start() noexcept override;

  void Shutdown() noexcept override;

 private:
  aimrt::LoggerRef GetLogger() { return core_.GetLogger(); }

  aimrt::co::Task<void> BenchStatisticsLoop();
  aimrt::co::Task<void> BenchLoop(int seq, std::atomic_bool& bench_run_flag);

  aimrt::co::Task<void> FixedFreqStatisticsLoop();
  aimrt::co::Task<void> FixedFreqLoop(int32_t freq, std::atomic_bool& bench_run_flag);

  aimrt::co::Task<void> WaitForServiceServer();

 private:
  Options options_;

  aimrt::CoreRef core_;
  aimrt::co::AsyncScope scope_;
  std::atomic_bool run_flag_ = true;

  std::shared_ptr<example_ros2::srv::RosTestRpcProxy> proxy_;

  std::string msg_;
  std::vector<std::vector<double> > time_consumption_statistics_;
};

}  // namespace aimrt::examples::example_ros2_rpc::ros2_rpc_perf_client_module
