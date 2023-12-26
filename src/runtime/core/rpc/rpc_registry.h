#pragma once

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "aimrt_module_c_interface/rpc/rpc_handle_base.h"
#include "aimrt_module_cpp_interface/rpc/rpc_context.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "util/log_util.h"

namespace aimrt::runtime::core::rpc {

struct ServiceFuncWrapper {
  std::string_view func_name;
  std::string_view pkg_path;
  std::string_view module_name;
  const void* custom_type_support_ptr = nullptr;
  const aimrt_type_support_base_t* req_type_support = nullptr;
  const aimrt_type_support_base_t* rsp_type_support = nullptr;
  aimrt::util::Function<aimrt_function_service_func_ops_t> service_func;
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
  RpcRegistry()
      : logger_ptr_(std::make_shared<common::util::LoggerWrapper>()) {}
  ~RpcRegistry() = default;

  RpcRegistry(const RpcRegistry&) = delete;
  RpcRegistry& operator=(const RpcRegistry&) = delete;

  void SetLogger(const std::shared_ptr<common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

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
  std::shared_ptr<common::util::LoggerWrapper> logger_ptr_;

  // pkg_path:module_name:func_name:wrapper
  using ServiceFuncMap = std::unordered_map<std::string_view, std::unique_ptr<ServiceFuncWrapper>>;
  using ServiceModuleMap = std::unordered_map<std::string_view, ServiceFuncMap>;
  using ServicePkgMap = std::unordered_map<std::string_view, ServiceModuleMap>;
  ServicePkgMap service_func_wrapper_map_;

  // pkg_path:module_name:func_name:wrapper
  using ClientFuncMap = std::unordered_map<std::string_view, std::unique_ptr<ClientFuncWrapper>>;
  using ClientModuleMap = std::unordered_map<std::string_view, ClientFuncMap>;
  using ClientPkgMap = std::unordered_map<std::string_view, ClientModuleMap>;
  ClientPkgMap client_func_wrapper_map_;
};
}  // namespace aimrt::runtime::core::rpc
