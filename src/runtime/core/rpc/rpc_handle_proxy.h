#pragma once

#include "aimrt_module_c_interface/rpc/rpc_handle_base.h"
#include "aimrt_module_cpp_interface/rpc/rpc_async_filter.h"
#include "core/rpc/rpc_backend_manager.h"

namespace aimrt::runtime::core::rpc {

class RpcHandleProxy {
 public:
  RpcHandleProxy(
      std::string_view pkg_path,
      std::string_view module_name,
      RpcBackendManager& rpc_backend_manager,
      aimrt::rpc::AsyncFilterManager& client_filter_manager,
      aimrt::rpc::AsyncFilterManager& server_filter_manager)
      : pkg_path_(pkg_path),
        module_name_(module_name),
        rpc_backend_manager_(rpc_backend_manager),
        client_filter_manager_(client_filter_manager),
        server_filter_manager_(server_filter_manager),
        base_(GenBase(this)) {}

  ~RpcHandleProxy() = default;

  RpcHandleProxy(const RpcHandleProxy&) = delete;
  RpcHandleProxy& operator=(const RpcHandleProxy&) = delete;

  const aimrt_rpc_handle_base_t* NativeHandle() const { return &base_; }

 private:
  bool RegisterServiceFunc(
      std::string_view func_name,
      const void* custom_type_support_ptr,
      const aimrt_type_support_base_t* req_type_support,
      const aimrt_type_support_base_t* rsp_type_support,
      aimrt::rpc::ServiceFunc&& service_func) noexcept {
    return rpc_backend_manager_.RegisterServiceFunc(
        ServiceFuncWrapper{
            .func_name = func_name,
            .pkg_path = pkg_path_,
            .module_name = module_name_,
            .custom_type_support_ptr = custom_type_support_ptr,
            .req_type_support = req_type_support,
            .rsp_type_support = rsp_type_support,
            .service_func =
                [this, service_func_ptr{std::make_shared<aimrt::rpc::ServiceFunc>(std::move(service_func))}](
                    aimrt::rpc::ContextRef ctx_ref,
                    const void* req_ptr,
                    void* rsp_ptr,
                    std::function<void(aimrt::rpc::Status)>&& callback) {
                  server_filter_manager_.InvokeRpc(
                      [service_func_ptr{std::move(service_func_ptr)}](
                          aimrt::rpc::ContextRef ctx_ref,
                          const void* req_ptr,
                          void* rsp_ptr,
                          std::function<void(aimrt::rpc::Status)>&& callback) {
                        aimrt::rpc::ServiceCallback service_callback(
                            [callback{std::move(callback)}](uint32_t status) {
                              callback(aimrt::rpc::Status(status));
                            });

                        (*service_func_ptr)(
                            ctx_ref.NativeHandle(),
                            req_ptr,
                            rsp_ptr,
                            service_callback.NativeHandle());
                      },
                      ctx_ref,
                      req_ptr,
                      rsp_ptr,
                      std::move(callback));
                }});
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
      aimrt::rpc::ClientCallback&& callback) noexcept {
    client_filter_manager_.InvokeRpc(
        [this, func_name](
            aimrt::rpc::ContextRef ctx_ref,
            const void* req_ptr,
            void* rsp_ptr,
            std::function<void(aimrt::rpc::Status)>&& callback) {
          rpc_backend_manager_.Invoke(
              ClientInvokeWrapper{
                  .func_name = func_name,
                  .pkg_path = pkg_path_,
                  .module_name = module_name_,
                  .ctx_ref = ctx_ref,
                  .req_ptr = req_ptr,
                  .rsp_ptr = rsp_ptr,
                  .callback = std::move(callback)});
        },
        ctx_ref,
        req_ptr,
        rsp_ptr,
        [callback_ptr{std::make_shared<aimrt::rpc::ClientCallback>(std::move(callback))}](aimrt::rpc::Status status) {
          (*callback_ptr)(status.Code());
        });
  }

  static aimrt_rpc_handle_base_t GenBase(void* impl) {
    return aimrt_rpc_handle_base_t{
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
              aimrt::rpc::ServiceFunc(service_func));
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
              aimrt::rpc::ClientCallback(callback));
        },
        .impl = impl};
  }

 private:
  const std::string pkg_path_;
  const std::string module_name_;

  RpcBackendManager& rpc_backend_manager_;
  aimrt::rpc::AsyncFilterManager& client_filter_manager_;
  aimrt::rpc::AsyncFilterManager& server_filter_manager_;

  aimrt_rpc_handle_base_t base_;
};

}  // namespace aimrt::runtime::core::rpc
