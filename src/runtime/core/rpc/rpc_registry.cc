#include "core/rpc/rpc_registry.h"

namespace aimrt::runtime::core::rpc {

bool RpcRegistry::RegisterServiceFunc(
    std::unique_ptr<ServiceFuncWrapper>&& service_func_wrapper_ptr) {
  std::string_view func_name = service_func_wrapper_ptr->func_name;
  std::string_view pkg_path = service_func_wrapper_ptr->pkg_path;
  std::string_view module_name = service_func_wrapper_ptr->module_name;

  auto& target_service_func_wrapper_ptr =
      service_func_wrapper_map_[pkg_path][module_name][func_name];
  if (target_service_func_wrapper_ptr) {
    AIMRT_ERROR(
        "Service func '{}' is registered repeatedly, module '{}', pkg path '{}'",
        func_name, module_name, pkg_path);
    return false;
  }

  target_service_func_wrapper_ptr = std::move(service_func_wrapper_ptr);

  service_index_map_[func_name].emplace_back(target_service_func_wrapper_ptr.get());

  AIMRT_TRACE(
      "Service func '{}' is successfully registered, module '{}', pkg path '{}'",
      func_name, module_name, pkg_path);
  return true;
}

bool RpcRegistry::RegisterClientFunc(
    std::unique_ptr<ClientFuncWrapper>&& client_func_wrapper_ptr) {
  std::string_view func_name = client_func_wrapper_ptr->func_name;
  std::string_view pkg_path = client_func_wrapper_ptr->pkg_path;
  std::string_view module_name = client_func_wrapper_ptr->module_name;

  auto& target_client_func_wrapper_ptr =
      client_func_wrapper_map_[pkg_path][module_name][func_name];
  if (target_client_func_wrapper_ptr) {
    AIMRT_ERROR(
        "Client func '{}' is registered repeatedly, module '{}', pkg path '{}'",
        func_name, module_name, pkg_path);
    return false;
  }

  target_client_func_wrapper_ptr = std::move(client_func_wrapper_ptr);

  client_index_map_[func_name].emplace_back(target_client_func_wrapper_ptr.get());

  AIMRT_TRACE(
      "Client func '{}' is successfully registered, module '{}', pkg path '{}'",
      func_name, module_name, pkg_path);
  return true;
}

}  // namespace aimrt::runtime::core::rpc
