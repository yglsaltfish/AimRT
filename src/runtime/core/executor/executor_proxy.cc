#include "core/executor/executor_proxy.h"
#include "core/global.h"

namespace aimrt::runtime::core::executor {

ExecutorProxy* ExecutorManagerProxy::GetExecutor(std::string_view executor_name) const {
  auto finditr = executor_proxy_map_.find(executor_name);
  if (finditr != executor_proxy_map_.end()) return finditr->second.get();

  AIMRT_WARN("Get executor failed, executor name '{}'", executor_name);

  return nullptr;
}

}  // namespace aimrt::runtime::core::executor
