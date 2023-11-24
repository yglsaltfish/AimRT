#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <stdexec/execution.hpp>

  #include "aimrt_module_cpp_interface/util/function.h"

namespace aimrt::co {

/**
 * @brief Convert an async callback function into a sender.
 * For detailed usage, please refer to test cases.
 *
 * @tparam Type of result, which is the parameter type of the callback function
 */
template <typename... Results>
class AsyncWrapper {
 public:
  using CallBack = aimrt::util::Function<void(Results &&...)>;
  using AsyncFunc = aimrt::util::Function<void(CallBack &&)>;

  static constexpr size_t ResultsSize = sizeof...(Results);

  template <typename Receiver>
    requires stdexec::receiver<Receiver>
  struct OperationState {
    template <typename Receiver2>
      requires std::constructible_from<Receiver, Receiver2>
    explicit OperationState(AsyncWrapper::AsyncFunc &&async_func, Receiver2 &&r)  //
        noexcept(std::is_nothrow_constructible_v<Receiver, Receiver2>)
        : async_func_((AsyncWrapper::AsyncFunc &&) async_func),
          receiver_((Receiver2 &&) r) {}

    friend void tag_invoke(stdexec::start_t, OperationState &op) noexcept {
      try {
        op.async_func_([r{(Receiver &&) op.receiver_}](Results &&...values) mutable {
          try {
            if constexpr (ResultsSize > 1) {
              stdexec::set_value((Receiver &&) r, std::tuple<Results...>{(Results &&) values...});
            } else if constexpr (ResultsSize == 1) {
              stdexec::set_value((Receiver &&) r, (Results &&) values...);
            } else {
              stdexec::set_value((Receiver &&) r);
            }
          } catch (...) {
            stdexec::set_error((Receiver &&) r, std::current_exception());
          }
        });
      } catch (...) {
        stdexec::set_error((Receiver &&) op.receiver_, std::current_exception());
      }
    }

    AsyncWrapper::AsyncFunc async_func_;
    Receiver receiver_;
  };

  template <typename F>
  explicit AsyncWrapper(F &&f) : async_func_((F &&) f) {}

  using is_sender = void;

  template <class Env>
  friend auto tag_invoke(
      stdexec::get_completion_signatures_t,
      const AsyncWrapper &,
      Env) noexcept {
    if constexpr (ResultsSize > 1) {
      return stdexec::completion_signatures<
          stdexec::set_value_t(std::tuple<Results...>),
          stdexec::set_error_t(std::exception_ptr)>{};
    } else if constexpr (ResultsSize == 1) {
      return stdexec::completion_signatures<
          stdexec::set_value_t(Results...),
          stdexec::set_error_t(std::exception_ptr)>{};
    } else {
      return stdexec::completion_signatures<
          stdexec::set_value_t(),
          stdexec::set_error_t(std::exception_ptr)>{};
    }
  }

  template <stdexec::receiver Receiver>
  friend auto tag_invoke(stdexec::connect_t, AsyncWrapper &&s, Receiver &&rec) {
    return OperationState<std::remove_cvref_t<Receiver>>(
        (AsyncFunc &&) s.async_func_, (Receiver &&) rec);
  }

  struct empty_env {};

  friend empty_env tag_invoke(stdexec::get_env_t, const AsyncWrapper &) noexcept {
    return {};
  }

 private:
  AsyncFunc async_func_;
};

}  // namespace aimrt::co

#endif
