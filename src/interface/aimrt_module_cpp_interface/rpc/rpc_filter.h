#pragma once

#ifdef AIMRT_USE_EXECUTOR

  #include <concepts>
  #include <list>
  #include <memory>

  #include "aimrt_module_cpp_interface/co/task.h"
  #include "aimrt_module_cpp_interface/rpc/rpc_context.h"
  #include "aimrt_module_cpp_interface/rpc/rpc_status.h"
  #include "aimrt_module_cpp_interface/util/function.h"

namespace aimrt {
namespace rpc {

using RpcHandle = Function<co::Task<Status>(ContextRef, const void*, void*)>;
using RpcFilter = Function<co::Task<Status>(ContextRef, const void*, void*, const RpcHandle&)>;

class FilterManager {
 public:
  FilterManager()
      : final_filter_([](ContextRef ctx_ref, const void* req, void* rsp, const RpcHandle& h) -> co::Task<Status> {
          return h(ctx_ref, req, rsp);
        }) {}
  ~FilterManager() = default;

  FilterManager(const FilterManager&) = delete;
  FilterManager& operator=(const FilterManager&) = delete;

  void RegisterFilter(RpcFilter&& filter) {
    final_filter_ =
        [final_filter{std::move(final_filter_)}, cur_filter{std::move(filter)}](
            ContextRef ctx_ref, const void* req, void* rsp, const RpcHandle& h) -> co::Task<Status> {
      co_return co_await cur_filter(
          ctx_ref, req, rsp,
          [&final_filter, &h](ContextRef ctx_ref, const void* req, void* rsp) -> co::Task<Status> {
            return final_filter(ctx_ref, req, rsp, h);
          });
    };
  }

  co::Task<Status> InvokeRpc(const RpcHandle& h, ContextRef ctx_ref, const void* req, void* rsp) {
    return final_filter_(ctx_ref, req, rsp, h);
  }

 private:
  RpcFilter final_filter_;
};

}  // namespace rpc
}  // namespace aimrt

#endif
