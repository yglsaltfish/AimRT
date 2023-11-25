#pragma once

#include <exec/timed_scheduler.hpp>
#include <stdexec/execution.hpp>

#include "aimrt_module_cpp_interface/executor/executor_manager.h"
#include "aimrt_module_cpp_interface/util/function.h"

namespace aimrt::co {

// Scheduler
class AimRTScheduler {
 public:
  // OperationState
  template <typename Receiver>
    requires stdexec::receiver<Receiver>
  struct OperationState final {
    template <typename Receiver2>
      requires std::constructible_from<Receiver, Receiver2>
    OperationState(executor::ExecutorRef executor_ref, Receiver2&& r)  //
        noexcept(std::is_nothrow_constructible_v<Receiver, Receiver2>)
        : executor_ref_(executor_ref), receiver_((Receiver2 &&) r) {}

    friend void tag_invoke(stdexec::start_t, OperationState& op) noexcept {
      op.executor_ref_.Execute([r{(Receiver &&) op.receiver_}]() mutable {
        try {
          stdexec::set_value((Receiver &&) r);
        } catch (...) {
          stdexec::set_error((Receiver &&) r, std::current_exception());
        }
      });
    }

    executor::ExecutorRef executor_ref_;
    Receiver receiver_;
  };

  // Sender
  class Task {
   public:
    using is_sender = void;
    using completion_signatures = stdexec::completion_signatures<
        stdexec::set_value_t(),
        stdexec::set_error_t(std::exception_ptr)>;

    explicit Task(executor::ExecutorRef executor_ref) noexcept
        : executor_ref_(executor_ref) {}

    template <class R>
    friend auto tag_invoke(stdexec::connect_t, const Task& self, R&& rec)  //
        noexcept(stdexec::__nothrow_constructible_from<stdexec::__decay_t<R>, R>) {
      return OperationState<std::remove_cvref_t<R>>(self.executor_ref_, (R &&) rec);
    }

    struct Env {
      executor::ExecutorRef executor_ref_;

      template <class CPO>
      friend AimRTScheduler
      tag_invoke(stdexec::get_completion_scheduler_t<CPO>, const Env& self) noexcept {
        return AimRTScheduler(self.executor_ref_);
      }
    };

    friend Env tag_invoke(stdexec::get_env_t, const Task& self) noexcept {
      return Env{self.executor_ref_};
    }

   private:
    executor::ExecutorRef executor_ref_;
  };

  // OperationState
  template <typename Receiver>
    requires stdexec::receiver<Receiver>
  struct SchedulerAfterOperationState final {
    template <typename Receiver2>
      requires std::constructible_from<Receiver, Receiver2>
    SchedulerAfterOperationState(
        executor::ExecutorRef executor_ref,
        std::chrono::steady_clock::duration dt,
        Receiver2&& r)  //
        noexcept(std::is_nothrow_constructible_v<Receiver, Receiver2>)
        : executor_ref_(executor_ref), dt_(dt), receiver_((Receiver2 &&) r) {}

    friend void tag_invoke(stdexec::start_t, SchedulerAfterOperationState& op) noexcept {
      op.executor_ref_.ExecuteAfter(op.dt_, [r{(Receiver &&) op.receiver_}]() mutable {
        try {
          stdexec::set_value((Receiver &&) r);
        } catch (...) {
          stdexec::set_error((Receiver &&) r, std::current_exception());
        }
      });
    }

   private:
    executor::ExecutorRef executor_ref_;
    std::chrono::steady_clock::duration dt_;
    Receiver receiver_;
  };

  // Sender
  class SchedulerAfterTask {
   public:
    using is_sender = void;
    using completion_signatures = stdexec::completion_signatures<
        stdexec::set_value_t(),
        stdexec::set_error_t(std::exception_ptr)>;

    SchedulerAfterTask(
        executor::ExecutorRef executor_ref,
        std::chrono::steady_clock::duration dt) noexcept
        : executor_ref_(executor_ref), dt_(dt) {}

    template <class R>
    friend auto tag_invoke(stdexec::connect_t, const SchedulerAfterTask& self, R&& rec)  //
        noexcept(stdexec::__nothrow_constructible_from<stdexec::__decay_t<R>, R>) {
      return SchedulerAfterOperationState<std::remove_cvref_t<R>>(self.executor_ref_, self.dt_, (R &&) rec);
    }

    struct Env {
      executor::ExecutorRef executor_ref_;

      template <class CPO>
      friend AimRTScheduler
      tag_invoke(stdexec::get_completion_scheduler_t<CPO>, const Env& self) noexcept {
        return AimRTScheduler(self.executor_ref_);
      }
    };

    friend Env tag_invoke(stdexec::get_env_t, const SchedulerAfterTask& self) noexcept {
      return Env{self.executor_ref_};
    }

   private:
    executor::ExecutorRef executor_ref_;
    std::chrono::steady_clock::duration dt_;
  };

 public:
  explicit AimRTScheduler(executor::ExecutorRef executor_ref) noexcept
      : executor_ref_(executor_ref) {}

  friend Task
  tag_invoke(stdexec::schedule_t, const AimRTScheduler& s) noexcept {
    return Task(s.executor_ref_);
  }

  friend std::chrono::steady_clock::time_point
  tag_invoke(exec::now_t, const AimRTScheduler&) noexcept {
    return std::chrono::steady_clock::now();
  }

  friend SchedulerAfterTask
  tag_invoke(exec::schedule_after_t,
             const AimRTScheduler& s,
             std::chrono::steady_clock::duration dt) noexcept {
    return SchedulerAfterTask(s.executor_ref_, dt);
  }

  friend SchedulerAfterTask
  tag_invoke(exec::schedule_at_t,
             const AimRTScheduler& s,
             const std::chrono::steady_clock::time_point& tp) noexcept {
    return SchedulerAfterTask(s.executor_ref_, tp - std::chrono::steady_clock::now());
  }

  friend bool operator==(const AimRTScheduler& a, const AimRTScheduler& b) noexcept {
    return a.executor_ref_.NativeHandle() == b.executor_ref_.NativeHandle();
  }

  friend bool operator!=(const AimRTScheduler& a, const AimRTScheduler& b) noexcept {
    return a.executor_ref_.NativeHandle() != b.executor_ref_.NativeHandle();
  }

 private:
  executor::ExecutorRef executor_ref_;
};

// Context
class AimRTContext {
 public:
  explicit AimRTContext(executor::ExecutorManagerRef executor_manager_ref) noexcept
      : executor_manager_ref_(executor_manager_ref) {}

  AimRTScheduler GetScheduler(std::string_view executor_name) {
    return AimRTScheduler(executor_manager_ref_.GetExecutor(executor_name));
  }

 private:
  executor::ExecutorManagerRef executor_manager_ref_;
};

}  // namespace aimrt::co
