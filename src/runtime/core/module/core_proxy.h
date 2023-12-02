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

  void SetConfigurator(const aimrt_configurator_base_t* ptr) {
    configurator_ = ptr;
  }

  void SetLogger(const aimrt_logger_base_t* ptr) {
    logger_ = ptr;
  }

  void SetExecutorManager(const aimrt_executor_manager_base_t* ptr) {
    executor_manager_ = ptr;
  }

  void SetRpcHandle(const aimrt_rpc_handle_base_t* ptr) {
    rpc_handle_ = ptr;
  }

  void SetChannel(const aimrt_channel_handle_base_t* ptr) {
    channel_handle_ = ptr;
  }

  void SetAllocator(const aimrt_allocator_base_t* ptr) {
    allocator_handle_ = ptr;
  }

  const aimrt_core_base_t* NativeHandle() const { return &base_; }

 private:
  static aimrt_core_base_t GenBase(void* impl) {
    return aimrt_core_base_t{
        .configurator = [](void* impl) -> const aimrt_configurator_base_t* {
          return static_cast<CoreProxy*>(impl)->configurator_;
        },
        .logger = [](void* impl) -> const aimrt_logger_base_t* {
          return static_cast<CoreProxy*>(impl)->logger_;
        },
        .executor_manager = [](void* impl) -> const aimrt_executor_manager_base_t* {
          return static_cast<CoreProxy*>(impl)->executor_manager_;
        },
        .rpc_handle = [](void* impl) -> const aimrt_rpc_handle_base_t* {
          return static_cast<CoreProxy*>(impl)->rpc_handle_;
        },
        .channel_handle = [](void* impl) -> const aimrt_channel_handle_base_t* {
          return static_cast<CoreProxy*>(impl)->channel_handle_;
        },
        .allocator_handle = [](void* impl) -> const aimrt_allocator_base_t* {
          return static_cast<CoreProxy*>(impl)->allocator_handle_;
        },
        .impl = impl};
  }

 private:
  const aimrt_configurator_base_t* configurator_;
  const aimrt_logger_base_t* logger_;
  const aimrt_executor_manager_base_t* executor_manager_;
  const aimrt_rpc_handle_base_t* rpc_handle_;
  const aimrt_channel_handle_base_t* channel_handle_;
  const aimrt_allocator_base_t* allocator_handle_;

  const aimrt_core_base_t base_;
};

}  // namespace aimrt::runtime::core::module
