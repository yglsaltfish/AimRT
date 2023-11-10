#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <unifex/async_scope.hpp>
  #include <unifex/sync_wait.hpp>
  #include <unifex/then.hpp>

namespace aimrt {
namespace co {

/**
 * @brief Detach executes a coroutine. Use a global async_scope
 *
 * @tparam Sender
 * @param sender
 */
template <typename Sender>
  requires unifex::sender<Sender>
inline void StartDetached(Sender&& sender) {
  struct AsyncScopeDeleter {
    void operator()(unifex::async_scope* p) {
      unifex::sync_wait(p->cleanup());
      delete p;
    }
  };
  static std::unique_ptr<unifex::async_scope, AsyncScopeDeleter> scope_ptr{
      new unifex::async_scope()};
  scope_ptr->spawn((Sender &&) sender);
}

/**
 * @brief Detach executes a coroutine and calls the callback at the end of the
 * coroutine
 *
 * @tparam Sender
 * @tparam CallBack
 * @param sender
 * @param callback
 */
template <typename Sender, typename CallBack>
  requires unifex::sender<Sender>
inline void StartDetached(Sender&& sender, CallBack&& callback) {
  StartDetached(((Sender &&) sender) | unifex::then((CallBack &&) callback));
}

}  // namespace co
}  // namespace aimrt

#endif