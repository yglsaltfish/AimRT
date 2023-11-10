#pragma once

#include <map>
#include <optional>

#include "core/executor/executor_base.h"
#include "core/global.h"

namespace aimrt::runtime::core::executor {

class ExecutorProxy {
 public:
  explicit ExecutorProxy(ExecutorBase* executor_ptr)
      : executor_ptr_(executor_ptr),
        base_(GenBase(this)) {}

  ~ExecutorProxy() = default;

  ExecutorProxy(const ExecutorProxy&) = delete;
  ExecutorProxy& operator=(const ExecutorProxy&) = delete;

  std::string_view Type() const { return executor_ptr_->Type(); }
  std::string_view Name() const { return executor_ptr_->Name(); }

  bool ThreadSafe() const { return executor_ptr_->ThreadSafe(); };
  bool IsInCurrentExecutor() const {
    return executor_ptr_->IsInCurrentExecutor();
  };

  void Execute(Function<aimrt_function_executor_task_ops_t>&& task) {
    executor_ptr_->Execute(std::move(task));
  }
  void ExecuteAfterNs(uint64_t dt,
                      Function<aimrt_function_executor_task_ops_t>&& task) {
    executor_ptr_->ExecuteAfterNs(dt, std::move(task));
  }
  void ExecuteAtNs(uint64_t tp,
                   Function<aimrt_function_executor_task_ops_t>&& task) {
    executor_ptr_->ExecuteAtNs(tp, std::move(task));
  }

  const aimrt_executor_base_t* NativeHandle() const { return &base_; }

 private:
  static aimrt_executor_base_t GenBase(void* impl) {
    return aimrt_executor_base_t{
        .type = [](void* impl) -> aimrt_string_view_t {
          return aimrt::ToAimRTStringView(
              static_cast<ExecutorProxy*>(impl)->Type());
        },
        .name = [](void* impl) -> aimrt_string_view_t {
          return aimrt::ToAimRTStringView(
              static_cast<ExecutorProxy*>(impl)->Name());
        },
        .is_thread_safe = [](void* impl) -> bool {
          return static_cast<ExecutorProxy*>(impl)->ThreadSafe();
        },
        .is_in_current_executor = [](void* impl) -> bool {
          return static_cast<ExecutorProxy*>(impl)->IsInCurrentExecutor();
        },
        .execute = [](void* impl, aimrt_function_base_t* task) {
          static_cast<ExecutorProxy*>(impl)->Execute(
              Function<aimrt_function_executor_task_ops_t>(task));  //
        },
        .execute_after_ns = [](void* impl, uint64_t dt, aimrt_function_base_t* task) {
          static_cast<ExecutorProxy*>(impl)->ExecuteAfterNs(
              dt, Function<aimrt_function_executor_task_ops_t>(task));  //
        },
        .execute_at_ns = [](void* impl, uint64_t tp, aimrt_function_base_t* task) {
          static_cast<ExecutorProxy*>(impl)->ExecuteAtNs(
              tp, Function<aimrt_function_executor_task_ops_t>(task));  //
        },
        .impl = impl};
  }

 private:
  ExecutorBase* executor_ptr_;

  const aimrt_executor_base_t base_;
};

class ExecutorManagerProxy {
 public:
  using ExecutorProxyMap =
      std::map<std::string, std::unique_ptr<ExecutorProxy>, std::less<> >;

 public:
  explicit ExecutorManagerProxy(const ExecutorProxyMap& executor_proxy_map)
      : executor_proxy_map_(executor_proxy_map),
        base_(GenBase(this)) {}

  ~ExecutorManagerProxy() = default;

  ExecutorManagerProxy(const ExecutorManagerProxy&) = delete;
  ExecutorManagerProxy& operator=(const ExecutorManagerProxy&) = delete;

  ExecutorProxy* GetExecutor(std::string_view executor_name) const {
    auto finditr = executor_proxy_map_.find(executor_name);
    if (finditr != executor_proxy_map_.end()) return finditr->second.get();

    AIMRT_WARN("Get executor failed, executor name '{}'", executor_name);

    return nullptr;
  }

  const aimrt_executor_manager_base_t* NativeHandle() const { return &base_; }

 private:
  static aimrt_executor_manager_base_t GenBase(void* impl) {
    return aimrt_executor_manager_base_t{
        .get_executor = [](void* impl, aimrt_string_view_t executor_name)
            -> const aimrt_executor_base_t* {
          auto ptr = static_cast<ExecutorManagerProxy*>(impl)->GetExecutor(
              aimrt::ToStdStringView(executor_name));
          return (ptr != nullptr) ? ptr->NativeHandle() : nullptr;
        },
        .impl = impl};
  }

 private:
  const ExecutorProxyMap& executor_proxy_map_;
  const aimrt_executor_manager_base_t base_;
};

}  // namespace aimrt::runtime::core::executor