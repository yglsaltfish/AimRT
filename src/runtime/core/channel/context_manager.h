#pragma once

#include "core/channel/context.h"
#include "util/log_util.h"

namespace aimrt::runtime::core::channel {

class ContextManager {
 public:
  ContextManager()
      : logger_ptr_(std::make_shared<aimrt::common::util::LoggerWrapper>()) {}
  ~ContextManager() = default;

  ContextManager(const ContextManager&) = delete;
  ContextManager& operator=(const ContextManager&) = delete;

  void SetLogger(const std::shared_ptr<aimrt::common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const aimrt::common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

  ContextImpl* NewContext();
  void DeleteContext(ContextImpl* ctx_ptr);

  std::shared_ptr<ContextImpl> NewContextSharedPtr() {
    return std::shared_ptr<ContextImpl>(NewContext(), [this](ContextImpl* ptr) { DeleteContext(ptr); });
  }

 private:
  std::shared_ptr<aimrt::common::util::LoggerWrapper> logger_ptr_;
};

}  // namespace aimrt::runtime::core::channel
