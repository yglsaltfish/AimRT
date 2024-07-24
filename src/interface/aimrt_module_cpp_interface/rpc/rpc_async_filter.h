#pragma once

#include <functional>

#include "aimrt_module_cpp_interface/rpc/rpc_context.h"
#include "aimrt_module_cpp_interface/rpc/rpc_status.h"

namespace aimrt::rpc {

using AsyncRpcHandle =
    std::function<void(ContextRef, const void*, void*, std::function<void(Status)>&&)>;
using AsyncRpcFilter =
    std::function<void(ContextRef, const void*, void*, std::function<void(Status)>&&, const AsyncRpcHandle&)>;

class AsyncFilterManager {
 public:
  AsyncFilterManager()
      : final_filter_(
            [](ContextRef ctx_ref,
               const void* req,
               void* rsp,
               std::function<void(Status)>&& callback,
               const AsyncRpcHandle& h) {
              h(ctx_ref, req, rsp, std::move(callback));
            }) {}
  ~AsyncFilterManager() = default;

  AsyncFilterManager(const AsyncFilterManager&) = delete;
  AsyncFilterManager& operator=(const AsyncFilterManager&) = delete;

  void RegisterFilter(AsyncRpcFilter&& filter) {
    final_filter_ =
        [final_filter{std::move(final_filter_)},
         cur_filter{std::move(filter)}](
            ContextRef ctx_ref,
            const void* req,
            void* rsp,
            std::function<void(Status)>&& callback,
            const AsyncRpcHandle& h) {
          cur_filter(
              ctx_ref,
              req,
              rsp,
              std::move(callback),
              [final_filter{std::move(final_filter)}, &h](
                  ContextRef ctx_ref,
                  const void* req,
                  void* rsp,
                  std::function<void(Status)>&& callback) {
                final_filter(ctx_ref, req, rsp, std::move(callback), h);
              });
        };
  }

  void InvokeRpc(
      const AsyncRpcHandle& h,
      ContextRef ctx_ref,
      const void* req,
      void* rsp,
      std::function<void(Status)>&& callback) const {
    return final_filter_(ctx_ref, req, rsp, std::move(callback), h);
  }

  void Clear() {
    final_filter_ = AsyncRpcFilter();
  }

 private:
  AsyncRpcFilter final_filter_;
};

}  // namespace aimrt::rpc