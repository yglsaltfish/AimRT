#pragma once

#include "aimrt_module_c_interface/rpc/rpc_handle_base.h"
#include "core/rpc/context.h"
#include "core/rpc/rpc_backend_manager.h"

namespace aimrt::runtime::core::rpc {

class RpcHandleProxy {
 public:
  RpcHandleProxy(
      std::string_view pkg_path,
      std::string_view module_name,
      RpcBackendManager& rpc_backend_manager,
      ContextManager& context_manager)
      : pkg_path_(pkg_path),
        module_name_(module_name),
        rpc_backend_manager_(rpc_backend_manager),
        context_manager_(context_manager) {
    base_ = aimrt_rpc_handle_base_t{
        .register_service_func = [](void* impl,
                                    aimrt_string_view_t func_name,
                                    const void* custom_type_support_ptr,
                                    const aimrt_type_support_base_t* req_type_support,
                                    const aimrt_type_support_base_t* rsp_type_support,
                                    aimrt_function_base_t* service_func) -> bool {
          return static_cast<RpcHandleProxy*>(impl)->RegisterServiceFunc(
              aimrt::util::ToStdStringView(func_name),
              custom_type_support_ptr,
              req_type_support,
              rsp_type_support,
              aimrt::util::Function<aimrt_function_service_func_ops_t>(service_func));
        },
        .register_client_func = [](void* impl,
                                   aimrt_string_view_t func_name,
                                   const void* custom_type_support_ptr,
                                   const aimrt_type_support_base_t* req_type_support,
                                   const aimrt_type_support_base_t* rsp_type_support) -> bool {
          return static_cast<RpcHandleProxy*>(impl)->RegisterClientFunc(
              aimrt::util::ToStdStringView(func_name),
              custom_type_support_ptr,
              req_type_support,
              rsp_type_support);
        },
        .invoke = [](void* impl,
                     aimrt_string_view_t func_name,
                     const aimrt_rpc_context_base_t* ctx_ptr,
                     const void* req_ptr,
                     void* rsp_ptr,
                     aimrt_function_base_t* callback) {  //
          static_cast<RpcHandleProxy*>(impl)->Invoke(
              aimrt::util::ToStdStringView(func_name),
              aimrt::rpc::ContextRef(ctx_ptr),
              req_ptr,
              rsp_ptr,
              aimrt::util::Function<aimrt_function_client_callback_ops_t>(callback));
        },
        .new_context = [](void* impl) -> const aimrt_rpc_context_base_t* {
          return static_cast<RpcHandleProxy*>(impl)->NewContextBase()->NativeHandle();
        },
        .delete_context = [](void* impl, const aimrt_rpc_context_base_t* ctx) {  //
          static_cast<RpcHandleProxy*>(impl)->DeleteContextBase(
              static_cast<ContextImpl*>(ctx->impl));
        },
        .impl = this};
  }
  ~RpcHandleProxy() = default;

  RpcHandleProxy(const RpcHandleProxy&) = delete;
  RpcHandleProxy& operator=(const RpcHandleProxy&) = delete;

  bool RegisterServiceFunc(
      std::string_view func_name,
      const void* custom_type_support_ptr,
      const aimrt_type_support_base_t* req_type_support,
      const aimrt_type_support_base_t* rsp_type_support,
      aimrt::util::Function<aimrt_function_service_func_ops_t>&& service_func) noexcept {
    return rpc_backend_manager_.RegisterServiceFunc(
        ServiceFuncWrapper{
            .func_name = func_name,
            .pkg_path = pkg_path_,
            .module_name = module_name_,
            .custom_type_support_ptr = custom_type_support_ptr,
            .req_type_support = req_type_support,
            .rsp_type_support = rsp_type_support,
            .service_func = std::move(service_func)});
  }

  bool RegisterClientFunc(
      std::string_view func_name,
      const void* custom_type_support_ptr,
      const aimrt_type_support_base_t* req_type_support,
      const aimrt_type_support_base_t* rsp_type_support) noexcept {
    return rpc_backend_manager_.RegisterClientFunc(
        ClientFuncWrapper{
            .func_name = func_name,
            .pkg_path = pkg_path_,
            .module_name = module_name_,
            .custom_type_support_ptr = custom_type_support_ptr,
            .req_type_support = req_type_support,
            .rsp_type_support = rsp_type_support});
  }

  void Invoke(
      std::string_view func_name,
      aimrt::rpc::ContextRef ctx_ref,
      const void* req_ptr,
      void* rsp_ptr,
      aimrt::util::Function<aimrt_function_client_callback_ops_t>&& callback) noexcept {
    rpc_backend_manager_.Invoke(
        ClientInvokeWrapper{
            .func_name = func_name,
            .pkg_path = pkg_path_,
            .module_name = module_name_,
            .ctx_ref = ctx_ref,
            .req_ptr = req_ptr,
            .rsp_ptr = rsp_ptr,
            .callback = std::move(callback)});
  }

  ContextImpl* NewContextBase() noexcept {
    return context_manager_.NewContext();
  }

  void DeleteContextBase(ContextImpl* ctx_ptr) noexcept {
    context_manager_.DeleteContext(ctx_ptr);
  }

  const aimrt_rpc_handle_base_t* NativeHandle() const { return &base_; }

 private:
  const std::string pkg_path_;
  const std::string module_name_;

  RpcBackendManager& rpc_backend_manager_;
  ContextManager& context_manager_;

  aimrt_rpc_handle_base_t base_;
};

}  // namespace aimrt::runtime::core::rpc
