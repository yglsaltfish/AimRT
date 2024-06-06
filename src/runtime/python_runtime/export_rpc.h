#pragma once

#include <future>

#include "aimrt_module_cpp_interface/rpc/rpc_handle.h"
#include "python_runtime/export_type_support.h"

#include "pybind11/pybind11.h"

namespace aimrt::runtime::python_runtime {

inline void ExportRpcStatus(pybind11::object m) {
  using namespace aimrt::rpc;

  pybind11::enum_<aimrt_rpc_status_code_t>(m, "RpcStatusRetCode")
      .value("OK", AIMRT_RPC_STATUS_OK)
      .value("UNKNOWN", AIMRT_RPC_STATUS_UNKNOWN)
      .value("TIMEOUT", AIMRT_RPC_STATUS_TIMEOUT)
      .value("SVR_UNKNOWN", AIMRT_RPC_STATUS_SVR_UNKNOWN)
      .value("SVR_NOT_IMPLEMENTED", AIMRT_RPC_STATUS_SVR_NOT_IMPLEMENTED)
      .value("SVR_NOT_FOUND", AIMRT_RPC_STATUS_SVR_NOT_FOUND)
      .value("SVR_INVALID_SERIALIZATION_TYPE", AIMRT_RPC_STATUS_SVR_INVALID_SERIALIZATION_TYPE)
      .value("SVR_SERIALIZATION_FAILDE", AIMRT_RPC_STATUS_SVR_SERIALIZATION_FAILDE)
      .value("SVR_INVALID_DESERIALIZATION_TYPE", AIMRT_RPC_STATUS_SVR_INVALID_DESERIALIZATION_TYPE)
      .value("SVR_DESERIALIZATION_FAILDE", AIMRT_RPC_STATUS_SVR_DESERIALIZATION_FAILDE)
      .value("SVR_HANDLE_FAILDE", AIMRT_RPC_STATUS_SVR_HANDLE_FAILDE)
      .value("CLI_UNKNOWN", AIMRT_RPC_STATUS_CLI_UNKNOWN)
      .value("CLI_INVALID_ADDR", AIMRT_RPC_STATUS_CLI_INVALID_ADDR)
      .value("CLI_INVALID_SERIALIZATION_TYPE", AIMRT_RPC_STATUS_CLI_INVALID_SERIALIZATION_TYPE)
      .value("CLI_SERIALIZATION_FAILDE", AIMRT_RPC_STATUS_CLI_SERIALIZATION_FAILDE)
      .value("CLI_INVALID_DESERIALIZATION_TYPE", AIMRT_RPC_STATUS_CLI_INVALID_DESERIALIZATION_TYPE)
      .value("CLI_DESERIALIZATION_FAILDE", AIMRT_RPC_STATUS_CLI_DESERIALIZATION_FAILDE)
      .value("CLI_NO_BACKEND_TO_HANDLE", AIMRT_RPC_STATUS_CLI_NO_BACKEND_TO_HANDLE)
      .value("CLI_SEND_REQ_FAILED", AIMRT_RPC_STATUS_CLI_SEND_REQ_FAILED);

  pybind11::class_<Status>(m, "RpcStatus")
      .def(pybind11::init<>())
      .def(pybind11::init<aimrt_rpc_status_code_t>())
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

inline pybind11::bytes rpc_empty_py_bytes;

inline void PyRpcServiceBaseRegisterServiceFunc(
    aimrt::rpc::ServiceBase& service,
    std::string_view func_name,
    std::shared_ptr<const PyTypeSupport> req_type_support,
    std::shared_ptr<const PyTypeSupport> rsp_type_support,
    std::function<std::tuple<aimrt::rpc::Status, std::string>(aimrt::rpc::ContextRef, const pybind11::bytes&)>&& service_func) {
  static std::vector<std::shared_ptr<const PyTypeSupport>> py_ts_vec;
  py_ts_vec.emplace_back(req_type_support);
  py_ts_vec.emplace_back(rsp_type_support);

  aimrt::util::Function<aimrt_function_service_func_ops_t> aimrt_service_func(
      [service_func{std::move(service_func)}](
          const aimrt_rpc_context_base_t* ctx, const void* req_ptr, void* rsp_ptr, aimrt_function_base_t* callback) {
        aimrt::util::Function<aimrt_function_service_callback_ops_t> callback_f(callback);

        aimrt::rpc::ContextRef ctx_ref(ctx);

        try {
          const std::string& req_buf = *static_cast<const std::string*>(req_ptr);
          std::string& rsp_buf = *static_cast<std::string*>(rsp_ptr);

          // TODO，未知原因，在此处使用空字符串构造pybind11::bytes时会挂掉。但是在外面构造没有问题
          if (req_buf.empty()) [[unlikely]] {
            auto [status, rsp_buf_tmp] = service_func(ctx_ref, rpc_empty_py_bytes);

            rsp_buf = std::move(rsp_buf_tmp);

            callback_f(status.Code());
            return;
          } else {
            auto req_buf_bytes = pybind11::bytes(req_buf);

            auto [status, rsp_buf_tmp] = service_func(ctx_ref, req_buf_bytes);

            pybind11::gil_scoped_acquire acquire;
            req_buf_bytes.release();
            pybind11::gil_scoped_release release;

            rsp_buf = std::move(rsp_buf_tmp);

            callback_f(status.Code());
            return;
          }

        } catch (const std::exception& e) {
          callback_f(AIMRT_RPC_STATUS_SVR_HANDLE_FAILDE);
          return;
        }
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

inline std::tuple<aimrt::rpc::Status, pybind11::bytes> PyRpcHandleRefInvoke(
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

  return {aimrt::rpc::Status(fu.get()), pybind11::bytes(rsp_buf)};
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