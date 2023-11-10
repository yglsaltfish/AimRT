#pragma once

#include <string>

#include "aimrt_module_c_interface/core_base.h"

namespace aimrt::runtime::core::module {

class CoreProxy {
 public:
  CoreProxy() : base_(GenBase(this)) {}
  ~CoreProxy() = default;

  CoreProxy(const CoreProxy&) = delete;
  CoreProxy& operator=(const CoreProxy&) = delete;

  void SetConfigurator(const aimrt_configurator_base_t* configurator_ptr) {
    configurator_ = configurator_ptr;
  }
  const aimrt_configurator_base_t* GetConfigurator() const {
    return configurator_;
  }

  void SetLogger(const aimrt_logger_base_t* logger_ptr) {
    logger_ = logger_ptr;
  }
  const aimrt_logger_base_t* GetLogger() const { return logger_; }

  void SetExecutorManager(const aimrt_executor_manager_base_t* executor_manager_ptr) {
    executor_manager_ = executor_manager_ptr;
  }
  const aimrt_executor_manager_base_t* GetExecutorManager() const {
    return executor_manager_;
  }

  void SetRpcHandle(const aimrt_rpc_handle_base_t* rpc_handle_ptr) {
    rpc_handle_ = rpc_handle_ptr;
  }
  const aimrt_rpc_handle_base_t* GetRpcHandle() const { return rpc_handle_; }

  void SetChannel(const aimrt_channel_handle_base_t* channel_ptr) {
    channel_handle_ = channel_ptr;
  }
  const aimrt_channel_handle_base_t* GetChannel() const {
    return channel_handle_;
  }

  const aimrt_core_base_t* NativeHandle() const { return &base_; }

 private:
  static aimrt_core_base_t GenBase(void* impl) {
    return aimrt_core_base_t{
        .configurator = [](void* impl) -> const aimrt_configurator_base_t* {
          return static_cast<CoreProxy*>(impl)->GetConfigurator();
        },
        .logger = [](void* impl) -> const aimrt_logger_base_t* {
          return static_cast<CoreProxy*>(impl)->GetLogger();
        },
        .executor_manager = [](void* impl) -> const aimrt_executor_manager_base_t* {
          return static_cast<CoreProxy*>(impl)->GetExecutorManager();
        },
        .rpc_handle = [](void* impl) -> const aimrt_rpc_handle_base_t* {
          return static_cast<CoreProxy*>(impl)->GetRpcHandle();
        },
        .channel_handle = [](void* impl) -> const aimrt_channel_handle_base_t* {
          return static_cast<CoreProxy*>(impl)->GetChannel();
        },
        .impl = impl};
  }

 private:
  const aimrt_configurator_base_t* configurator_;
  const aimrt_logger_base_t* logger_;
  const aimrt_executor_manager_base_t* executor_manager_;
  const aimrt_rpc_handle_base_t* rpc_handle_;
  const aimrt_channel_handle_base_t* channel_handle_;

  const aimrt_core_base_t base_;
};

}  // namespace aimrt::runtime::core::module
