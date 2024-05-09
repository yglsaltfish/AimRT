#pragma once

#include <chrono>
#include <stdexcept>
#include <string_view>

#include "aimrt_module_c_interface/executor/executor_base.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "util/exception.h"
#include "util/time_util.h"

namespace aimrt::executor {

class ExecutorRef {
 public:
  using Task = aimrt::util::Function<aimrt_function_executor_task_ops_t>;

  ExecutorRef() = default;
  explicit ExecutorRef(const aimrt_executor_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~ExecutorRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_executor_base_t* NativeHandle() const { return base_ptr_; }

  std::string_view Type() const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return aimrt::util::ToStdStringView(base_ptr_->type(base_ptr_->impl));
  }

  std::string_view Name() const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return aimrt::util::ToStdStringView(base_ptr_->name(base_ptr_->impl));
  }

  bool ThreadSafe() const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return base_ptr_->is_thread_safe(base_ptr_->impl);
  }

  bool IsInCurrentExecutor() const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return base_ptr_->is_in_current_executor(base_ptr_->impl);
  }

  bool SupportTimerSchedule() const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return base_ptr_->is_support_timer_schedule(base_ptr_->impl);
  }

  void Execute(Task&& task) const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    base_ptr_->execute(base_ptr_->impl, task.NativeHandle());
  }

  std::chrono::system_clock::time_point Now() const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");
    return std::chrono::system_clock::time_point(
        std::chrono::duration_cast<std::chrono::system_clock::time_point::duration>(
            std::chrono::nanoseconds(base_ptr_->now(base_ptr_->impl))));
  }

  void ExecuteAt(std::chrono::system_clock::time_point tp, Task&& task) const {
    AIMRT_ASSERT(base_ptr_, "Reference is null.");

    AIMRT_ASSERT(SupportTimerSchedule(), "Current executor does not support timer scheduling.");

    base_ptr_->execute_at_ns(
        base_ptr_->impl,
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                tp.time_since_epoch())
                .count()),
        task.NativeHandle());
  }

  void ExecuteAfter(std::chrono::nanoseconds dt, Task&& task) const {
    ExecuteAt(
        Now() + std::chrono::duration_cast<std::chrono::system_clock::time_point::duration>(dt),
        std::move(task));
  }

 private:
  const aimrt_executor_base_t* base_ptr_ = nullptr;
};

}  // namespace aimrt::executor
