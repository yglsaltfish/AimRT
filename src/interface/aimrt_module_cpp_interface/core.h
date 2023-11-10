#pragma once

#include "aimrt_module_c_interface/core_base.h"
#include "aimrt_module_cpp_interface/channel/channel_handle.h"
#include "aimrt_module_cpp_interface/configurator/configurator.h"
#include "aimrt_module_cpp_interface/executor/executor_manager.h"
#include "aimrt_module_cpp_interface/logger/logger.h"
#include "aimrt_module_cpp_interface/rpc/rpc_handle.h"

namespace aimrt {

/**
 * @brief Abstract of framework runtime, providing some functions for modules
 *
 */
class CoreRef {
 public:
  CoreRef() = default;
  explicit CoreRef(const aimrt_core_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~CoreRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_core_base_t* NativeHandle() const { return base_ptr_; }

  /**
   * @brief Get configurator handle
   *
   * @return ConfiguratorRef
   */
  ConfiguratorRef GetConfigurator() const {
    assert(base_ptr_);
    return ConfiguratorRef(base_ptr_->configurator(base_ptr_->impl));
  }

  /**
   * @brief Get logger handle
   *
   * @return LoggerRef
   */
  LoggerRef GetLogger() const {
    assert(base_ptr_);
    return LoggerRef(base_ptr_->logger(base_ptr_->impl));
  }

  /**
   * @brief Get executor manager handle
   *
   * @return ExecutorManagerRef
   */
  ExecutorManagerRef GetExecutorManager() const {
    assert(base_ptr_);
    return ExecutorManagerRef(base_ptr_->executor_manager(base_ptr_->impl));
  }

  /**
   * @brief Get rpc handle
   *
   * @return rpc::RpcHandleRef
   */
  rpc::RpcHandleRef GetRpcHandle() const {
    assert(base_ptr_);
    return rpc::RpcHandleRef(base_ptr_->rpc_handle(base_ptr_->impl));
  }

  /**
   * @brief Get channel handle
   *
   * @return channel::ChannelHandleRef
   */
  channel::ChannelHandleRef GetChannel() const {
    assert(base_ptr_);
    return channel::ChannelHandleRef(base_ptr_->channel_handle(base_ptr_->impl));
  }

 private:
  const aimrt_core_base_t* base_ptr_ = nullptr;
};

}  // namespace aimrt
