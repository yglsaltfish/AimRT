#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>

#include "aimrt_module_c_interface/rpc/rpc_handle_base.h"
#include "aimrt_module_cpp_interface/rpc/rpc_context.h"
#include "aimrt_module_cpp_interface/util/function.h"

namespace aimrt::runtime::core::rpc {

struct ServiceFuncWrapper {
  std::string_view func_name;
  std::string_view pkg_path;
  std::string_view module_name;
  const void* custom_type_support_ptr = nullptr;
  const aimrt_type_support_base_t* req_type_support = nullptr;
  const aimrt_type_support_base_t* rsp_type_support = nullptr;
  Function<aimrt_function_service_func_ops_t> service_func;
};

struct ClientFuncWrapper {
  std::string_view func_name;
  std::string_view pkg_path;
  std::string_view module_name;
  const void* custom_type_support_ptr = nullptr;
  const aimrt_type_support_base_t* req_type_support = nullptr;
  const aimrt_type_support_base_t* rsp_type_support = nullptr;
};

class RpcRegistry {
 public:
  RpcRegistry() = default;
  ~RpcRegistry() = default;

  RpcRegistry(const RpcRegistry&) = delete;
  RpcRegistry& operator=(const RpcRegistry&) = delete;

  bool RegisterServiceFunc(
      std::unique_ptr<ServiceFuncWrapper>&& service_func_wrapper_ptr);

  bool RegisterClientFunc(
      std::unique_ptr<ClientFuncWrapper>&& client_func_wrapper_ptr);

  const auto& GetServiceFuncWrapperMap() const {
    return service_func_wrapper_map_;
  }

  const auto& GetClientFuncWrapperMap() const {
    return client_func_wrapper_map_;
  }

 private:
  // pkg_path:module_name:func_name:wrapper
  using ServiceFuncMap = std::map<std::string_view, std::unique_ptr<ServiceFuncWrapper> >;
  using ServiceModuleMap = std::map<std::string_view, ServiceFuncMap>;
  using ServicePkgMap = std::map<std::string_view, ServiceModuleMap>;
  ServicePkgMap service_func_wrapper_map_;

  // pkg_path:module_name:func_name:wrapper
  using ClientFuncMap = std::map<std::string_view, std::unique_ptr<ClientFuncWrapper> >;
  using ClientModuleMap = std::map<std::string_view, ClientFuncMap>;
  using ClientPkgMap = std::map<std::string_view, ClientModuleMap>;
  ClientPkgMap client_func_wrapper_map_;
};
}  // namespace aimrt::runtime::core::rpc
