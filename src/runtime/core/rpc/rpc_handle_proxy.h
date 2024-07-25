#pragma once

#include "aimrt_module_c_interface/rpc/rpc_handle_base.h"
#include "core/rpc/rpc_backend_manager.h"

namespace aimrt::runtime::core::rpc {

class RpcHandleProxy {
 public:
  RpcHandleProxy(
      std::string_view pkg_path,
      std::string_view module_name,
      RpcBackendManager& rpc_backend_manager,
      const std::unordered_set<std::string>& passed_context_meta_keys)
      : pkg_path_(pkg_path),
        module_name_(module_name),
        rpc_backend_manager_(rpc_backend_manager),
        passed_context_meta_keys_(passed_context_meta_keys),
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
                  aimrt::rpc::ServiceCallback service_callback(
                      [callback{std::move(callback)}](uint32_t status) {
                        callback(aimrt::rpc::Status(status));
                      });

                  (*service_func_ptr)(
                      ctx_ref.NativeHandle(),
                      req_ptr,
                      rsp_ptr,
                      service_callback.NativeHandle());
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
    rpc_backend_manager_.Invoke(
        ClientInvokeWrapper{
            .func_name = func_name,
            .pkg_path = pkg_path_,
            .module_name = module_name_,
            .ctx_ref = ctx_ref,
            .req_ptr = req_ptr,
            .rsp_ptr = rsp_ptr,
            .callback =
                [callback_ptr{std::make_shared<aimrt::rpc::ClientCallback>(std::move(callback))}](
                    aimrt::rpc::Status status) {
                  (*callback_ptr)(status.Code());
                }});
  }

  void MergeServerContextToClientContext(
      aimrt::rpc::ContextRef server_ctx_ref, aimrt::rpc::ContextRef client_ctx_ref) {
    if (server_ctx_ref.GetType() != aimrt_rpc_context_type_t::AIMRT_RPC_SERVER_CONTEXT ||
        client_ctx_ref.GetType() != aimrt_rpc_context_type_t::AIMRT_RPC_CLIENT_CONTEXT) [[unlikely]] {
      // TODO warn log
      return;
    }

    for (const auto& key : passed_context_meta_keys_) {
      auto val = server_ctx_ref.GetMetaValue(key);
      if (!val.empty()) client_ctx_ref.SetMetaValue(key, val);
    }
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
        .merge_server_context_to_client_context = [](void* impl,                                        //
                                                     const aimrt_rpc_context_base_t* server_ctx_ptr,    //
                                                     const aimrt_rpc_context_base_t* client_ctx_ptr) {  //
          static_cast<RpcHandleProxy*>(impl)->MergeServerContextToClientContext(
              aimrt::rpc::ContextRef(server_ctx_ptr), aimrt::rpc::ContextRef(client_ctx_ptr));
        },
        .impl = impl};
  }

 private:
  const std::string pkg_path_;
  const std::string module_name_;

  RpcBackendManager& rpc_backend_manager_;

  const std::unordered_set<std::string>& passed_context_meta_keys_;

  const aimrt_rpc_handle_base_t base_;
};

}  // namespace aimrt::runtime::core::rpc
