#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "aimrt_module_c_interface/rpc/rpc_handle_base.h"
#include "aimrt_module_cpp_interface/rpc/rpc_filter.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "aimrt_module_cpp_interface/util/string.h"

namespace aimrt::rpc {

class ServiceBase {
  friend class RpcHandleRef;

 public:
  template <typename T>
    requires std::constructible_from<RpcFilter, T>
  void RegisterFilter(T&& filter) {
    filter_mgr_.RegisterFilter((T &&) filter);
  }

 protected:
  struct ServiceFuncWrapper {
    const void* custom_type_support_ptr;
    const aimrt_type_support_base_t* req_type_support;
    const aimrt_type_support_base_t* rsp_type_support;
    aimrt::util::Function<aimrt_function_service_func_ops_t> service_func;
  };

 protected:
  ServiceBase() = default;
  virtual ~ServiceBase() = default;

  ServiceBase(const ServiceBase&) = delete;
  ServiceBase& operator=(const ServiceBase&) = delete;

  void RegisterServiceFunc(
      std::string_view func_name,
      const void* custom_type_support_ptr,
      const aimrt_type_support_base_t* req_type_support,
      const aimrt_type_support_base_t* rsp_type_support,
      aimrt::util::Function<aimrt_function_service_func_ops_t>&& service_func) {
    service_func_wrapper_map_.emplace(
        func_name,
        ServiceFuncWrapper{
            .custom_type_support_ptr = custom_type_support_ptr,
            .req_type_support = req_type_support,
            .rsp_type_support = rsp_type_support,
            .service_func = std::move(service_func)});
  }

 protected:
  std::unordered_map<std::string_view, ServiceFuncWrapper> service_func_wrapper_map_;
  FilterManager filter_mgr_;
};

class RpcHandleRef {
 public:
  RpcHandleRef() = default;
  explicit RpcHandleRef(const aimrt_rpc_handle_base_t* base_ptr)
      : base_ptr_(base_ptr) {}
  ~RpcHandleRef() = default;

  explicit operator bool() const { return (base_ptr_ != nullptr); }

  const aimrt_rpc_handle_base_t* NativeHandle() const { return base_ptr_; }

  /**
   * @brief Register service
   *
   * @param service_ptr
   * @return Register result
   */
  bool RegisterService(ServiceBase* service_ptr) {
    assert(base_ptr_);

    for (auto& itr : service_ptr->service_func_wrapper_map_) {
      if (!base_ptr_->register_service_func(
              base_ptr_->impl,
              aimrt::util::ToAimRTStringView(itr.first),
              itr.second.custom_type_support_ptr,
              itr.second.req_type_support,
              itr.second.rsp_type_support,
              itr.second.service_func.NativeHandle()))
        return false;
    }

    return true;
  }

  /**
   * @brief Register client func
   *
   * @param func_name
   * @param custom_type_support_ptr
   * @param req_type_support
   * @param rsp_type_support
   * @return Register result
   */
  bool RegisterClientFunc(
      std::string_view func_name,
      const void* custom_type_support_ptr,
      const aimrt_type_support_base_t* req_type_support,
      const aimrt_type_support_base_t* rsp_type_support) {
    assert(base_ptr_);
    return base_ptr_->register_client_func(
        base_ptr_->impl,
        aimrt::util::ToAimRTStringView(func_name),
        custom_type_support_ptr,
        req_type_support,
        rsp_type_support);
  }

  /**
   * @brief Invoke rpc
   *
   * @param func_name
   * @param ctx_ptr
   * @param req_ptr
   * @param rsp_ptr
   * @param callback
   */
  void Invoke(
      std::string_view func_name,
      ContextRef ctx_ref,
      const void* req_ptr,
      void* rsp_ptr,
      aimrt::util::Function<aimrt_function_client_callback_ops_t>&& callback) const {
    assert(base_ptr_);
    base_ptr_->invoke(
        base_ptr_->impl,
        aimrt::util::ToAimRTStringView(func_name),
        ctx_ref.NativeHandle(),
        req_ptr,
        rsp_ptr,
        callback.NativeHandle());
  }

  /**
   * @brief Create context shared ptr
   *
   * @return ContextSharedPtr
   */
  ContextSharedPtr NewContextSharedPtr() {
    assert(base_ptr_);
    return std::shared_ptr<const aimrt_rpc_context_base_t>(
        base_ptr_->new_context(base_ptr_->impl),
        [base_ptr_ = base_ptr_](const aimrt_rpc_context_base_t* ptr) {
          base_ptr_->delete_context(base_ptr_->impl, ptr);
        });
  }

 private:
  const aimrt_rpc_handle_base_t* base_ptr_ = nullptr;
};

class ProxyBase {
 public:
  template <typename T>
    requires std::constructible_from<RpcFilter, T>
  void RegisterFilter(T&& filter) {
    filter_mgr_.RegisterFilter((T &&) filter);
  }

 protected:
  explicit ProxyBase(RpcHandleRef rpc_handle_ref)
      : rpc_handle_ref_(rpc_handle_ref) {}
  virtual ~ProxyBase() = default;

  ProxyBase(const ProxyBase&) = delete;
  ProxyBase& operator=(const ProxyBase&) = delete;

 protected:
  RpcHandleRef rpc_handle_ref_;
  FilterManager filter_mgr_;
};

template <class ProxyType>
bool RegisterClientFunc(RpcHandleRef rpc_handle_ref) {
  return ProxyType::RegisterClientFunc(rpc_handle_ref);
}

}  // namespace aimrt::rpc
