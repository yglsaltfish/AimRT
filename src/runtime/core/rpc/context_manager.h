#pragma once

#include "core/rpc/context.h"

namespace aimrt::runtime::core::rpc {

class ContextManager {
 public:
  ContextManager() = default;
  ~ContextManager() = default;

  ContextManager(const ContextManager&) = delete;
  ContextManager& operator=(const ContextManager&) = delete;

  ContextImpl* NewContext();
  void DeleteContext(ContextImpl* ctx_ptr);
};

}  // namespace aimrt::runtime::core::rpc
