#pragma once

#include <unordered_map>

#include "core/executor/executor_base.h"
#include "util/string_util.h"

namespace aimrt::runtime::core::executor {

class ExecutorProxy {
 public:
  explicit ExecutorProxy(ExecutorBase* executor_ptr)
      : base_(GenBase(executor_ptr)) {}

  ~ExecutorProxy() = default;

  ExecutorProxy(const ExecutorProxy&) = delete;
  ExecutorProxy& operator=(const ExecutorProxy&) = delete;

  const aimrt_executor_base_t* NativeHandle() const { return &base_; }

 private:
  static aimrt_executor_base_t GenBase(void* impl) {
    return aimrt_executor_base_t{
        .type = [](void* impl) -> aimrt_string_view_t {
          return aimrt::util::ToAimRTStringView(
              static_cast<ExecutorBase*>(impl)->Type());
        },
        .name = [](void* impl) -> aimrt_string_view_t {
          return aimrt::util::ToAimRTStringView(
              static_cast<ExecutorBase*>(impl)->Name());
        },
        .is_thread_safe = [](void* impl) -> bool {
          return static_cast<ExecutorBase*>(impl)->ThreadSafe();
        },
        .is_in_current_executor = [](void* impl) -> bool {
          return static_cast<ExecutorBase*>(impl)->IsInCurrentExecutor();
        },
        .is_support_timer_schedule = [](void* impl) -> bool {
          return static_cast<ExecutorBase*>(impl)->SupportTimerSchedule();
        },
        .execute = [](void* impl, aimrt_function_base_t* task) {
          static_cast<ExecutorBase*>(impl)->Execute(aimrt::executor::Task(task));  //
        },
        .now = [](void* impl) -> uint64_t {
          return static_cast<uint64_t>(
              std::chrono::duration_cast<std::chrono::nanoseconds>(
                  static_cast<ExecutorBase*>(impl)->Now().time_since_epoch())
                  .count());
        },
        .execute_at_ns = [](void* impl, uint64_t tp, aimrt_function_base_t* task) {
          static_cast<ExecutorBase*>(impl)->ExecuteAt(
              std::chrono::system_clock::time_point(
                  std::chrono::duration_cast<std::chrono::system_clock::time_point::duration>(
                      std::chrono::nanoseconds(tp))),
              aimrt::executor::Task(task));  //
        },
        .impl = impl};
  }

 private:
  const aimrt_executor_base_t base_;
};

class ExecutorManagerProxy {
 public:
  using ExecutorProxyMap = std::unordered_map<
      std::string, std::unique_ptr<ExecutorProxy>, aimrt::common::util::StringHash, std::equal_to<>>;

 public:
  explicit ExecutorManagerProxy(const ExecutorProxyMap& executor_proxy_map)
      : executor_proxy_map_(executor_proxy_map),
        base_(GenBase(this)) {}

  ~ExecutorManagerProxy() = default;

  ExecutorManagerProxy(const ExecutorManagerProxy&) = delete;
  ExecutorManagerProxy& operator=(const ExecutorManagerProxy&) = delete;

  const aimrt_executor_manager_base_t* NativeHandle() const { return &base_; }

 private:
  ExecutorProxy* GetExecutor(std::string_view executor_name) const {
    auto finditr = executor_proxy_map_.find(executor_name);
    if (finditr != executor_proxy_map_.end()) return finditr->second.get();

    // TODO，使找不到executor时打一个日志
    // AIMRT_WARN("Get executor failed, executor name '{}'", executor_name);

    return nullptr;
  }

  static aimrt_executor_manager_base_t GenBase(void* impl) {
    return aimrt_executor_manager_base_t{
        .get_executor = [](void* impl, aimrt_string_view_t executor_name)
            -> const aimrt_executor_base_t* {
          auto ptr = static_cast<ExecutorManagerProxy*>(impl)->GetExecutor(
              aimrt::util::ToStdStringView(executor_name));
          return (ptr != nullptr) ? ptr->NativeHandle() : nullptr;
        },
        .impl = impl};
  }

 private:
  const ExecutorProxyMap& executor_proxy_map_;
  const aimrt_executor_manager_base_t base_;
};

}  // namespace aimrt::runtime::core::executor