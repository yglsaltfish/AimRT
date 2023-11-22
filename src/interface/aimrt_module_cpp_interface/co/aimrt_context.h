#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/execute.hpp>

  #include "aimrt_module_cpp_interface/executor/executor_manager.h"
  #include "aimrt_module_cpp_interface/util/function.h"

namespace aimrt::co {

// Scheduler
class AimRTScheduler {
 public:
  // OperationState
  template <typename Receiver>
  struct OperationState final {
    template <typename Receiver2>
      requires std::constructible_from<Receiver, Receiver2>
    explicit OperationState(executor::ExecutorRef executor_ref, Receiver2&& r) noexcept(
        std::is_nothrow_constructible_v<Receiver, Receiver2>)
        : executor_ref_(executor_ref), receiver_((Receiver2 &&) r) {}

    void start() noexcept {
      executor_ref_.Execute([r{std::move(receiver_)}]() mutable {
        try {
          unifex::set_value(std::move(r));
        } catch (...) {
          unifex::set_error(std::move(r), std::current_exception());
        }
      });
    }

    executor::ExecutorRef executor_ref_;
    Receiver receiver_;
  };

  // Sender
  class Task {
   public:
    template <template <typename...> class Variant,
              template <typename...> class Tuple>
    using value_types = Variant<Tuple<>>;

    template <template <typename...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = false;

    explicit Task(executor::ExecutorRef executor_ref) noexcept
        : executor_ref_(executor_ref) {}

    template <typename Receiver>
    OperationState<unifex::remove_cvref_t<Receiver>> connect(Receiver&& receiver) {
      return OperationState<unifex::remove_cvref_t<Receiver>>(
          executor_ref_, (Receiver &&) receiver);
    }

   private:
    executor::ExecutorRef executor_ref_;
  };

  // OperationState
  template <typename Receiver>
  struct SchedulerAfterOperationState final {
    template <typename Receiver2>
      requires std::constructible_from<Receiver, Receiver2>
    explicit SchedulerAfterOperationState(
        executor::ExecutorRef executor_ref,
        const std::chrono::steady_clock::duration& dt,
        Receiver2&& r)  //
        noexcept(std::is_nothrow_constructible_v<Receiver, Receiver2>)
        : executor_ref_(executor_ref), dt_(dt), receiver_((Receiver2 &&) r) {}

    void start() noexcept {
      executor_ref_.ExecuteAfter(dt_, [r{std::move(receiver_)}]() mutable {
        try {
          unifex::set_value(std::move(r));
        } catch (...) {
          unifex::set_error(std::move(r), std::current_exception());
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
    template <template <typename...> class Variant,
              template <typename...> class Tuple>
    using value_types = Variant<Tuple<>>;

    template <template <typename...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = false;

    explicit SchedulerAfterTask(
        executor::ExecutorRef executor_ref,
        const std::chrono::steady_clock::duration& dt) noexcept
        : executor_ref_(executor_ref), dt_(dt) {}

    template <typename Receiver>
    SchedulerAfterOperationState<unifex::remove_cvref_t<Receiver>> connect(Receiver&& receiver) {
      return SchedulerAfterOperationState<unifex::remove_cvref_t<Receiver>>(
          executor_ref_, dt_, (Receiver &&) receiver);
    }

   private:
    executor::ExecutorRef executor_ref_;
    std::chrono::steady_clock::duration dt_;
  };

  // OperationState
  template <typename Receiver>
  struct SchedulerAtOperationState final {
    template <typename Receiver2>
      requires std::constructible_from<Receiver, Receiver2>
    explicit SchedulerAtOperationState(
        executor::ExecutorRef executor_ref,
        const std::chrono::steady_clock::time_point& tp,
        Receiver2&& r)  //
        noexcept(std::is_nothrow_constructible_v<Receiver, Receiver2>)
        : executor_ref_(executor_ref), tp_(tp), receiver_((Receiver2 &&) r) {}

    void start() noexcept {
      executor_ref_.ExecuteAt(tp_, [r{std::move(receiver_)}]() mutable {
        try {
          unifex::set_value(std::move(r));
        } catch (...) {
          unifex::set_error(std::move(r), std::current_exception());
        }
      });
    }

   private:
    executor::ExecutorRef executor_ref_;
    std::chrono::steady_clock::time_point tp_;
    Receiver receiver_;
  };

  // Sender
  class SchedulerAtTask {
   public:
    template <template <typename...> class Variant,
              template <typename...> class Tuple>
    using value_types = Variant<Tuple<>>;

    template <template <typename...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = false;

    explicit SchedulerAtTask(
        executor::ExecutorRef executor_ref,
        const std::chrono::steady_clock::time_point& tp) noexcept
        : executor_ref_(executor_ref), tp_(tp) {}

    template <typename Receiver>
    SchedulerAtOperationState<unifex::remove_cvref_t<Receiver>> connect(Receiver&& receiver) {
      return SchedulerAtOperationState<unifex::remove_cvref_t<Receiver>>(
          executor_ref_, tp_, (Receiver &&) receiver);
    }

   private:
    executor::ExecutorRef executor_ref_;
    std::chrono::steady_clock::time_point tp_;
  };

 public:
  explicit AimRTScheduler(executor::ExecutorRef executor_ref) noexcept
      : executor_ref_(executor_ref) {}

  Task schedule() const noexcept { return Task(executor_ref_); }

  SchedulerAfterTask schedule_after(
      const std::chrono::steady_clock::duration& dt) const noexcept {
    return SchedulerAfterTask(executor_ref_, dt);
  }

  SchedulerAtTask schedule_at(
      const std::chrono::steady_clock::time_point& tp) const noexcept {
    return SchedulerAtTask(executor_ref_, tp);
  }

  friend bool operator==(AimRTScheduler a, AimRTScheduler b) noexcept {
    return a.executor_ref_.NativeHandle() == b.executor_ref_.NativeHandle();
  }

  friend bool operator!=(AimRTScheduler a, AimRTScheduler b) noexcept {
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

#endif
