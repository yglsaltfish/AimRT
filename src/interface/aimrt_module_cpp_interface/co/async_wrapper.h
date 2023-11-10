#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/execute.hpp>

  #include "aimrt_module_cpp_interface/util/function.h"

namespace aimrt {
namespace co {

/**
 * @brief Convert an async callback function into a sender. For detailed usage,
 * please refer to test cases
 *
 * @tparam Type of result, which is the parameter type of the callback function
 */
template <typename... Results>
class AsyncWrapper {
 public:
  using CallBack = Function<void(Results &&...)>;
  using AsyncFunc = Function<void(CallBack)>;

  template <typename Receiver>
    requires unifex::receiver<Receiver>
  struct OperationState {
    template <typename Receiver2>
      requires std::constructible_from<Receiver, Receiver2>
    explicit OperationState(
        AsyncWrapper::AsyncFunc &&async_func,
        Receiver2 &&r)  //
        noexcept(std::is_nothrow_constructible_v<Receiver, Receiver2>)
        : async_func_((AsyncWrapper::AsyncFunc &&) async_func),
          receiver_((Receiver2 &&) r) {}

    void start() noexcept {
      try {
        async_func_([r{std::move(receiver_)}](Results &&...values) mutable {
          try {
            unifex::set_value(std::move(r), (Results &&) values...);
          } catch (...) {
            unifex::set_error(std::move(r), std::current_exception());
          }
        });
      } catch (...) {
        unifex::set_error(std::move(receiver_), std::current_exception());
      }
    }

    AsyncWrapper::AsyncFunc async_func_;
    Receiver receiver_;
  };

  template <template <typename...> class Variant,
            template <typename...> class Tuple>
  using value_types = Variant<Tuple<Results...>>;

  template <template <typename...> class Variant>
  using error_types = Variant<std::exception_ptr>;

  static constexpr bool sends_done = false;

  template <typename F>
  explicit AsyncWrapper(F &&f) : async_func_((F &&) f) {}

  template <typename Receiver>
  OperationState<unifex::remove_cvref_t<Receiver>> connect(Receiver &&receiver) {
    return OperationState<unifex::remove_cvref_t<Receiver>>(
        (AsyncFunc &&) async_func_, (Receiver &&) receiver);
  }

 private:
  AsyncFunc async_func_;
};

}  // namespace co
}  // namespace aimrt

#endif
