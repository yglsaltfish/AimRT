#pragma once

#include <future>

#include "aimrt_module_cpp_interface/co/async_wrapper.h"
#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/start_detached.h"
#include "aimrt_module_cpp_interface/co/then.h"
#include "aimrt_module_cpp_interface/rpc/rpc_handle.h"
#include "python_runtime/export_type_support.h"

#include "pybind11/pybind11.h"

namespace aimrt::runtime::python_runtime {

inline void ExportRpcStatus(pybind11::object m) {
  using namespace aimrt::rpc;

  pybind11::enum_<Status::RetCode>(m, "RpcStatusRetCode")
      .value("OK", Status::RetCode::OK)
      .value("UNKNOWN", Status::RetCode::UNKNOWN)
      .value("TIMEOUT", Status::RetCode::TIMEOUT)
      .value("SVR_UNKNOWN", Status::RetCode::SVR_UNKNOWN)
      .value("SVR_NOT_IMPLEMENTED", Status::RetCode::SVR_NOT_IMPLEMENTED)
      .value("SVR_NOT_FOUND", Status::RetCode::SVR_NOT_FOUND)
      .value("SVR_INVALID_SERIALIZATION_TYPE", Status::RetCode::SVR_INVALID_SERIALIZATION_TYPE)
      .value("SVR_SERIALIZATION_FAILDE", Status::RetCode::SVR_SERIALIZATION_FAILDE)
      .value("SVR_INVALID_DESERIALIZATION_TYPE", Status::RetCode::SVR_INVALID_DESERIALIZATION_TYPE)
      .value("SVR_DESERIALIZATION_FAILDE", Status::RetCode::SVR_DESERIALIZATION_FAILDE)
      .value("CLI_UNKNOWN", Status::RetCode::CLI_UNKNOWN)
      .value("CLI_INVALID_ADDR", Status::RetCode::CLI_INVALID_ADDR)
      .value("CLI_INVALID_SERIALIZATION_TYPE", Status::RetCode::CLI_INVALID_SERIALIZATION_TYPE)
      .value("CLI_SERIALIZATION_FAILDE", Status::RetCode::CLI_SERIALIZATION_FAILDE)
      .value("CLI_INVALID_DESERIALIZATION_TYPE", Status::RetCode::CLI_INVALID_DESERIALIZATION_TYPE)
      .value("CLI_DESERIALIZATION_FAILDE", Status::RetCode::CLI_DESERIALIZATION_FAILDE)
      .value("CLI_NO_BACKEND_TO_HANDLE", Status::RetCode::CLI_NO_BACKEND_TO_HANDLE)
      .value("CLI_SEND_REQ_FAILED", Status::RetCode::CLI_SEND_REQ_FAILED);

  pybind11::class_<Status>(m, "RpcStatus")
      .def(pybind11::init<>())
      .def(pybind11::init<Status::RetCode>())
      .def(pybind11::init<uint32_t>())
      .def("OK", &Status::OK)
      .def("__bool__", &Status::operator bool)
      .def("Code", &Status::Code)
      .def("ToString", &Status::ToString);
}

struct PyRpcContext {
  aimrt::rpc::ContextSharedPtr ctx_ptr;
};

inline void ExportRpcContextRef(pybind11::object m) {
  using namespace aimrt::rpc;

  pybind11::class_<PyRpcContext>(m, "RpcContext")
      .def(pybind11::init<>());

  pybind11::class_<ContextRef>(m, "RpcContextRef")
      .def(pybind11::init<>())
      .def(pybind11::init([](PyRpcContext ctx) {
        return new ContextRef(ctx.ctx_ptr);
      }))
      .def("__bool__", &ContextRef::operator bool)
      .def("Deadline", &ContextRef::Deadline)
      .def("SetDeadline", &ContextRef::SetDeadline)
      .def("Timeout", &ContextRef::Timeout)
      .def("SetTimeout", &ContextRef::SetTimeout)
      .def("GetMetaValue", &ContextRef::GetMetaValue)
      .def("SetMetaValue", &ContextRef::SetMetaValue)
      .def("GetFromAddr", &ContextRef::GetFromAddr)
      .def("GetToAddr", &ContextRef::GetToAddr)
      .def("SetToAddr", &ContextRef::SetToAddr)
      .def("GetSerializationType", &ContextRef::GetSerializationType)
      .def("SetSerializationType", &ContextRef::SetSerializationType);
}

inline void PyRpcServiceBaseRegisterServiceFunc(
    aimrt::rpc::ServiceBase& service,
    std::string_view func_name,
    std::shared_ptr<const PyTypeSupport> req_type_support,
    std::shared_ptr<const PyTypeSupport> rsp_type_support,
    std::function<std::tuple<aimrt::rpc::Status, std::string>(aimrt::rpc::ContextRef, const std::string&)>&& service_func) {
  static std::vector<std::shared_ptr<const PyTypeSupport>> py_ts_vec;
  py_ts_vec.emplace_back(req_type_support);
  py_ts_vec.emplace_back(rsp_type_support);

  aimrt::util::Function<aimrt_function_service_func_ops_t> aimrt_service_func(
      [service_ptr{&service}, service_func{std::move(service_func)}](
          const aimrt_rpc_context_base_t* ctx, const void* req, void* rsp, aimrt_function_base_t* callback) {
        static const aimrt::rpc::RpcHandle h =
            [&service_func](aimrt::rpc::ContextRef ctx_ref, const void* req_ptr, void* rsp_ptr)
            -> aimrt::co::Task<aimrt::rpc::Status> {
          const std::string& req_buf = *static_cast<const std::string*>(req_ptr);
          std::string& rsp_buf = *static_cast<std::string*>(rsp_ptr);

          auto [status, rsp_buf_tmp] = service_func(ctx_ref, req_buf);
          rsp_buf = std::move(rsp_buf_tmp);

          co_return status;
        };

        aimrt::util::Function<aimrt_function_service_callback_ops_t> f(callback);

        aimrt::co::StartDetached(
            aimrt::co::On(
                aimrt::co::InlineScheduler(),
                service_ptr->GetFilterManager().InvokeRpc(h, aimrt::rpc::ContextRef(ctx), req, rsp)) |
            aimrt::co::Then(
                [f{std::move(f)}](aimrt::rpc::Status status) {
                  f(status.Code());
                }));
      });

  service.RegisterServiceFunc(
      func_name,
      nullptr,
      req_type_support->NativeHandle(),
      rsp_type_support->NativeHandle(),
      std::move(aimrt_service_func));
}

inline void ExportRpcServiceBase(pybind11::object m) {
  using namespace aimrt::rpc;

  pybind11::class_<ServiceBase>(m, "ServiceBase")
      .def(pybind11::init<>())
      .def("RegisterServiceFunc", &PyRpcServiceBaseRegisterServiceFunc);
}

inline bool PyRpcHandleRefRegisterClientFunc(
    aimrt::rpc::RpcHandleRef& rpc_handle_ref,
    std::string_view func_name,
    std::shared_ptr<const PyTypeSupport> req_type_support,
    std::shared_ptr<const PyTypeSupport> rsp_type_support) {
  static std::vector<std::shared_ptr<const PyTypeSupport>> py_ts_vec;
  py_ts_vec.emplace_back(req_type_support);
  py_ts_vec.emplace_back(rsp_type_support);

  return rpc_handle_ref.RegisterClientFunc(
      func_name,
      nullptr,
      req_type_support->NativeHandle(),
      rsp_type_support->NativeHandle());
}

inline std::tuple<aimrt::rpc::Status, std::string> PyRpcHandleRefInvoke(
    aimrt::rpc::RpcHandleRef& rpc_handle_ref,
    std::string_view func_name,
    aimrt::rpc::ContextRef ctx_ref,
    const std::string& req_buf) {
  pybind11::gil_scoped_release release;

  std::string rsp_buf;
  std::promise<uint32_t> status_promise;

  aimrt::util::Function<aimrt_function_client_callback_ops_t> callback(
      [&status_promise](uint32_t status) {
        status_promise.set_value(status);
      });

  rpc_handle_ref.Invoke(
      func_name,
      ctx_ref,
      static_cast<const void*>(&req_buf),
      static_cast<void*>(&rsp_buf),
      std::move(callback));

  auto fu = status_promise.get_future();
  fu.wait();

  pybind11::gil_scoped_acquire acquire;

  return {aimrt::rpc::Status(fu.get()), rsp_buf};
}

inline PyRpcContext PyRpcHandleRefNewContextSharedPtr(aimrt::rpc::RpcHandleRef& rpc_handle_ref) {
  return PyRpcContext{rpc_handle_ref.NewContextSharedPtr()};
}

inline void ExportRpcHandleRef(pybind11::object m) {
  using namespace aimrt::rpc;

  pybind11::class_<RpcHandleRef>(m, "RpcHandleRef")
      .def(pybind11::init<>())
      .def("__bool__", &RpcHandleRef::operator bool)
      .def("RegisterService", &RpcHandleRef::RegisterService)
      .def("RegisterClientFunc", &PyRpcHandleRefRegisterClientFunc)
      .def("Invoke", &PyRpcHandleRefInvoke)
      .def("NewContextSharedPtr", &PyRpcHandleRefNewContextSharedPtr);
}
}  // namespace aimrt::runtime::python_runtime