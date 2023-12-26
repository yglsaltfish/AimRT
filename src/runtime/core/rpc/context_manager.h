#pragma once

#include "core/rpc/context.h"
#include "util/log_util.h"

namespace aimrt::runtime::core::rpc {

class ContextManager {
 public:
  ContextManager()
      : logger_ptr_(std::make_shared<common::util::LoggerWrapper>()) {}
  ~ContextManager() = default;

  ContextManager(const ContextManager&) = delete;
  ContextManager& operator=(const ContextManager&) = delete;

  void SetLogger(const std::shared_ptr<common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

  ContextImpl* NewContext();
  void DeleteContext(ContextImpl* ctx_ptr);

 private:
  std::shared_ptr<common::util::LoggerWrapper> logger_ptr_;
};

}  // namespace aimrt::runtime::core::rpc
