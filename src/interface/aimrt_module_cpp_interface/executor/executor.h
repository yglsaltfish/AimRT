#pragma once

#include <cassert>
#include <chrono>
#include <string_view>

#include "aimrt_module_c_interface/executor/executor_base.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "aimrt_module_cpp_interface/util/string.h"

namespace aimrt {

class ExecutorRef {
 public:
  using Task = Function<aimrt_function_executor_task_ops_t>;

  ExecutorRef() = default;
  explicit ExecutorRef(const aimrt_executor_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~ExecutorRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_executor_base_t* NativeHandle() const { return base_ptr_; }

  std::string_view Type() const {
    assert(base_ptr_);
    return ToStdStringView(base_ptr_->type(base_ptr_->impl));
  }

  std::string_view Name() const {
    assert(base_ptr_);
    return ToStdStringView(base_ptr_->name(base_ptr_->impl));
  }

  bool ThreadSafe() const {
    assert(base_ptr_);
    return base_ptr_->is_thread_safe(base_ptr_->impl);
  }

  bool IsInCurrentExecutor() const {
    assert(base_ptr_);
    return base_ptr_->is_in_current_executor(base_ptr_->impl);
  }

  void Execute(Task&& task) {
    assert(base_ptr_);
    base_ptr_->execute(base_ptr_->impl, task.NativeHandle());
  }

  void ExecuteAfter(const std::chrono::steady_clock::duration& dt, Task&& task) {
    assert(base_ptr_);
    base_ptr_->execute_after_ns(
        base_ptr_->impl,
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(dt).count()),
        task.NativeHandle());
  }

  void ExecuteAt(const std::chrono::steady_clock::time_point& tp, Task&& task) {
    assert(base_ptr_);
    base_ptr_->execute_at_ns(
        base_ptr_->impl,
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                tp.time_since_epoch())
                .count()),
        task.NativeHandle());
  }

 private:
  const aimrt_executor_base_t* base_ptr_ = nullptr;
};

}  // namespace aimrt
