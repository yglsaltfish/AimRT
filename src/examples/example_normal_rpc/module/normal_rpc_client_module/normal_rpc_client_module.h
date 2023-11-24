#pragma once

#include <atomic>
#include <memory>

#include "aimrt_module_cpp_interface/co/async_scope.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/module_base.h"
#include "aimrt_module_cpp_interface/rpc/rpc_filter.h"

#include "rpc.aimrt_rpc.pb.h"

namespace aimrt::examples::example_normal_rpc::normal_rpc_client_module {

class NormalRpcClientModule : public aimrt::ModuleBase {
 public:
  NormalRpcClientModule() = default;
  ~NormalRpcClientModule() override = default;

  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "NormalRpcClientModule"};
  }

  bool Initialize(aimrt::CoreRef core) noexcept override;

  bool Start() noexcept override;

  void Shutdown() noexcept override;

 private:
  aimrt::logger::LoggerRef GetLogger() { return core_.GetLogger(); }

  aimrt::co::Task<void> MainLoop();

 private:
  aimrt::CoreRef core_;
  aimrt::executor::ExecutorRef executor_;

  aimrt::co::AsyncScope scope_;
  std::atomic_bool run_flag_ = true;

  double rpc_frq_;
  std::shared_ptr<aimrt::protocols::example::ExampleServiceProxy> proxy_;
};

}  // namespace aimrt::examples::example_normal_rpc::normal_rpc_client_module
