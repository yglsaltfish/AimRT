#pragma once

#include <functional>

#include "aimrt_module_cpp_interface/rpc/rpc_context.h"
#include "aimrt_module_cpp_interface/rpc/rpc_status.h"
#include "aimrt_module_cpp_interface/util/type_support.h"

namespace aimrt::runtime::core::rpc {

struct FrameworkFilterData {
  std::string_view func_name;
  std::string_view pkg_path;
  std::string_view module_name;
  const void* custom_type_support_ptr;
  aimrt::util::TypeSupportRef req_type_support_ref;
  aimrt::util::TypeSupportRef rsp_type_support_ref;
};

using FrameworkAsyncRpcHandle =
    std::function<void(const FrameworkFilterData&, aimrt::rpc::ContextRef, const void*, void*, std::function<void(aimrt::rpc::Status)>&&)>;
using FrameworkAsyncRpcFilter =
    std::function<void(const FrameworkFilterData&, aimrt::rpc::ContextRef, const void*, void*, std::function<void(aimrt::rpc::Status)>&&, const FrameworkAsyncRpcHandle&)>;

class FrameworkAsyncFilterManager {
 public:
  FrameworkAsyncFilterManager()
      : final_filter_(
            [](const FrameworkFilterData& filter_data,
               aimrt::rpc::ContextRef ctx_ref,
               const void* req,
               void* rsp,
               std::function<void(aimrt::rpc::Status)>&& callback,
               const FrameworkAsyncRpcHandle& h) {
              h(filter_data, ctx_ref, req, rsp, std::move(callback));
            }) {}
  ~FrameworkAsyncFilterManager() = default;

  FrameworkAsyncFilterManager(const FrameworkAsyncFilterManager&) = delete;
  FrameworkAsyncFilterManager& operator=(const FrameworkAsyncFilterManager&) = delete;

  void RegisterFilter(FrameworkAsyncRpcFilter&& filter) {
    final_filter_ =
        [final_filter{std::move(final_filter_)},
         cur_filter{std::move(filter)}](
            const FrameworkFilterData& filter_data,
            aimrt::rpc::ContextRef ctx_ref,
            const void* req,
            void* rsp,
            std::function<void(aimrt::rpc::Status)>&& callback,
            const FrameworkAsyncRpcHandle& h) {
          cur_filter(
              filter_data,
              ctx_ref,
              req,
              rsp,
              std::move(callback),
              [final_filter{std::move(final_filter)}, &h](
                  const FrameworkFilterData& filter_data,
                  aimrt::rpc::ContextRef ctx_ref,
                  const void* req,
                  void* rsp,
                  std::function<void(aimrt::rpc::Status)>&& callback) {
                final_filter(filter_data, ctx_ref, req, rsp, std::move(callback), h);
              });
        };
  }

  void InvokeRpc(
      const FrameworkAsyncRpcHandle& h,
      const FrameworkFilterData& filter_data,
      aimrt::rpc::ContextRef ctx_ref,
      const void* req,
      void* rsp,
      std::function<void(aimrt::rpc::Status)>&& callback) const {
    return final_filter_(filter_data, ctx_ref, req, rsp, std::move(callback), h);
  }

  void Clear() {
    final_filter_ = FrameworkAsyncRpcFilter();
  }

 private:
  FrameworkAsyncRpcFilter final_filter_;
};

}  // namespace aimrt::runtime::core::rpc