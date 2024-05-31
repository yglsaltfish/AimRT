#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import sys

from google.protobuf.compiler import plugin_pb2 as plugin
from google.protobuf.compiler.plugin_pb2 import CodeGeneratorRequest as CodeGeneratorRequest
from google.protobuf.compiler.plugin_pb2 import CodeGeneratorResponse as CodeGeneratorResponse
from google.protobuf.descriptor_pb2 import FileDescriptorProto


class AimRTCodeGenerator(object):
    t_hfile_one_sync_service_func: str = r"""
  virtual aimrt::rpc::Status {{rpc_func_name}}(
      aimrt::rpc::ContextRef ctx_ref,
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp) {
    return aimrt::rpc::Status(AIMRT_RPC_STATUS_SVR_NOT_IMPLEMENTED);
  }"""

    t_hfile_one_sync_service_class: str = r"""
class {{service_name}}SyncService : public aimrt::rpc::ServiceBase {
 public:
  {{service_name}}SyncService();
  ~{{service_name}}SyncService() override = default;
{{hfile_sync_service_func}}
};"""

    t_hfile_one_async_service_func: str = r"""
  virtual void {{rpc_func_name}}(
      aimrt::rpc::ContextRef ctx_ref,
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp,
      aimrt::util::Function<void(aimrt::rpc::Status)>&& callback) {
    callback(aimrt::rpc::Status(AIMRT_RPC_STATUS_SVR_NOT_IMPLEMENTED));
  }"""

    t_hfile_one_async_service_class: str = r"""
class {{service_name}}AsyncService : public aimrt::rpc::ServiceBase {
 public:
  {{service_name}}AsyncService();
  ~{{service_name}}AsyncService() override = default;
{{hfile_async_service_func}}
};"""

    t_hfile_one_service_func: str = r"""
  virtual aimrt::co::Task<aimrt::rpc::Status> {{rpc_func_name}}(
      aimrt::rpc::ContextRef ctx_ref,
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp) {
    co_return aimrt::rpc::Status(AIMRT_RPC_STATUS_SVR_NOT_IMPLEMENTED);
  }"""

    t_hfile_one_service_class: str = r"""
class {{service_name}}CoService : public aimrt::rpc::CoServiceBase {
 public:
  {{service_name}}CoService();
  ~{{service_name}}CoService() override = default;
{{hfile_service_func}}
};

using {{service_name}} [[deprecated("Using {{service_name}}CoService.")]] = {{service_name}}CoService;"""

    t_hfile_one_service_register_client_func: str = r"""
bool Register{{service_name}}ClientFunc(aimrt::rpc::RpcHandleRef rpc_handle_ref);"""

    t_hfile_one_service_sync_proxy_func: str = r"""
  aimrt::rpc::Status {{rpc_func_name}}(
      aimrt::rpc::ContextRef ctx_ref,
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp);

  aimrt::rpc::Status {{rpc_func_name}}(
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp) {
    return {{rpc_func_name}}(aimrt::rpc::ContextRef(), req, rsp);
  }"""

    t_hfile_one_service_sync_proxy_class: str = r"""
class {{service_name}}SyncProxy : public aimrt::rpc::ProxyBase {
 public:
  explicit {{service_name}}SyncProxy(aimrt::rpc::RpcHandleRef rpc_handle_ref)
      : aimrt::rpc::ProxyBase(rpc_handle_ref) {}
  ~{{service_name}}SyncProxy() = default;

  static bool RegisterClientFunc(aimrt::rpc::RpcHandleRef rpc_handle_ref) {
    return Register{{service_name}}ClientFunc(rpc_handle_ref);
  }
{{hfile_service_sync_proxy_func}}
};"""

    t_hfile_one_service_async_proxy_func: str = r"""
  void {{rpc_func_name}}(
      aimrt::rpc::ContextRef ctx_ref,
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp,
      aimrt::util::Function<void(aimrt::rpc::Status)>&& callback);

  void {{rpc_func_name}}(
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp,
      aimrt::util::Function<void(aimrt::rpc::Status)>&& callback) {
    {{rpc_func_name}}(aimrt::rpc::ContextRef(), req, rsp, std::move(callback));
  }"""

    t_hfile_one_service_async_proxy_class: str = r"""
class {{service_name}}AsyncProxy : public aimrt::rpc::ProxyBase {
 public:
  explicit {{service_name}}AsyncProxy(aimrt::rpc::RpcHandleRef rpc_handle_ref)
      : aimrt::rpc::ProxyBase(rpc_handle_ref) {}
  ~{{service_name}}AsyncProxy() = default;

  static bool RegisterClientFunc(aimrt::rpc::RpcHandleRef rpc_handle_ref) {
    return Register{{service_name}}ClientFunc(rpc_handle_ref);
  }
{{hfile_service_async_proxy_func}}
};"""

    t_hfile_one_service_future_proxy_func: str = r"""
  std::future<aimrt::rpc::Status> {{rpc_func_name}}(
      aimrt::rpc::ContextRef ctx_ref,
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp);

  std::future<aimrt::rpc::Status> {{rpc_func_name}}(
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp) {
    return {{rpc_func_name}}(aimrt::rpc::ContextRef(), req, rsp);
  }"""

    t_hfile_one_service_future_proxy_class: str = r"""
class {{service_name}}FutureProxy : public aimrt::rpc::ProxyBase {
 public:
  explicit {{service_name}}FutureProxy(aimrt::rpc::RpcHandleRef rpc_handle_ref)
      : aimrt::rpc::ProxyBase(rpc_handle_ref) {}
  ~{{service_name}}FutureProxy() = default;

  static bool RegisterClientFunc(aimrt::rpc::RpcHandleRef rpc_handle_ref) {
    return Register{{service_name}}ClientFunc(rpc_handle_ref);
  }
{{hfile_service_future_proxy_func}}
};"""

    t_hfile_one_service_proxy_func: str = r"""
  aimrt::co::Task<aimrt::rpc::Status> {{rpc_func_name}}(
      aimrt::rpc::ContextRef ctx_ref,
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp);

  aimrt::co::Task<aimrt::rpc::Status> {{rpc_func_name}}(
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp) {
    return {{rpc_func_name}}(aimrt::rpc::ContextRef(), req, rsp);
  }"""

    t_hfile_one_service_proxy_class: str = r"""
class {{service_name}}CoProxy : public aimrt::rpc::CoProxyBase {
 public:
  explicit {{service_name}}CoProxy(aimrt::rpc::RpcHandleRef rpc_handle_ref)
      : aimrt::rpc::CoProxyBase(rpc_handle_ref) {}
  ~{{service_name}}CoProxy() = default;

  static bool RegisterClientFunc(aimrt::rpc::RpcHandleRef rpc_handle_ref) {
    return Register{{service_name}}ClientFunc(rpc_handle_ref);
  }
{{hfile_service_proxy_func}}
};

using {{service_name}}Proxy [[deprecated("Using {{service_name}}CoProxy.")]] = {{service_name}}CoProxy;"""

    t_hfile: str = r"""/**
 * @file {{file_name}}.aimrt_rpc.pb.h
 * @brief This file was generated by protoc-gen-aimrt_rpc which is a self-defined pb compiler plugin, do not edit it!!!
 */
#pragma once

#include <future>

#include "aimrt_module_cpp_interface/rpc/rpc_handle.h"
#include "aimrt_module_cpp_interface/rpc/rpc_status.h"

#include "aimrt_module_cpp_interface/co/task.h"

#include "{{file_name}}.pb.h"

{{namespace_begin}}
{{hfile_sync_service_class}}
{{hfile_async_service_class}}
{{hfile_service_class}}
{{hfile_service_register_client_func}}
{{hfile_service_sync_proxy_class}}
{{hfile_service_async_proxy_class}}
{{hfile_service_future_proxy_class}}
{{hfile_service_proxy_class}}
{{namespace_end}}
"""

    t_ccfile_one_sync_service_register_func: str = r"""
  {
    aimrt::util::Function<aimrt_function_service_func_ops_t> service_callback(
        [this](const aimrt_rpc_context_base_t* ctx, const void* req, void* rsp, aimrt_function_base_t* result_callback_ptr) {
          aimrt::util::Function<aimrt_function_service_callback_ops_t> result_callback(result_callback_ptr);

          auto status = {{rpc_func_name}}(
              aimrt::rpc::ContextRef(ctx),
              *static_cast<const {{rpc_req_name}}*>(req),
              *static_cast<{{rpc_rsp_name}}*>(rsp));

          result_callback(status.Code());
        });
    RegisterServiceFunc(
        "pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
        nullptr,
        aimrt::GetProtobufMessageTypeSupport<{{rpc_req_name}}>(),
        aimrt::GetProtobufMessageTypeSupport<{{rpc_rsp_name}}>(),
        std::move(service_callback));
  }"""

    t_ccfile_one_sync_service_class: str = r"""
{{service_name}}SyncService::{{service_name}}SyncService() {
{{ccfile_sync_service_register_func}}
}
"""

    t_ccfile_one_async_service_register_func: str = r"""
  {
    aimrt::util::Function<aimrt_function_service_func_ops_t> service_callback(
        [this](const aimrt_rpc_context_base_t* ctx, const void* req, void* rsp, aimrt_function_base_t* result_callback_ptr) {
          aimrt::util::Function<aimrt_function_service_callback_ops_t> result_callback(result_callback_ptr);

          {{rpc_func_name}}(
              aimrt::rpc::ContextRef(ctx),
              *static_cast<const {{rpc_req_name}}*>(req),
              *static_cast<{{rpc_rsp_name}}*>(rsp),
              [result_callback{std::move(result_callback)}](aimrt::rpc::Status status) {
                result_callback(status.Code());
              });
        });
    RegisterServiceFunc(
        "pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
        nullptr,
        aimrt::GetProtobufMessageTypeSupport<{{rpc_req_name}}>(),
        aimrt::GetProtobufMessageTypeSupport<{{rpc_rsp_name}}>(),
        std::move(service_callback));
  }"""

    t_ccfile_one_async_service_class: str = r"""
{{service_name}}AsyncService::{{service_name}}AsyncService() {
{{ccfile_async_service_register_func}}
}
"""

    t_ccfile_one_service_register_func: str = r"""
  {
    aimrt::util::Function<aimrt_function_service_func_ops_t> service_callback(
        [this](const aimrt_rpc_context_base_t* ctx, const void* req, void* rsp, aimrt_function_base_t* result_callback_ptr) {
          const aimrt::rpc::RpcHandle h =
              [this](aimrt::rpc::ContextRef ctx_ref, const void* req_ptr, void* rsp_ptr)
              -> aimrt::co::Task<aimrt::rpc::Status> {
            return {{rpc_func_name}}(
                ctx_ref,
                *static_cast<const {{rpc_req_name}}*>(req_ptr),
                *static_cast<{{rpc_rsp_name}}*>(rsp_ptr));
          };

          aimrt::util::Function<aimrt_function_service_callback_ops_t> result_callback(result_callback_ptr);

          aimrt::co::StartDetached(
              aimrt::co::On(
                  aimrt::co::InlineScheduler(),
                  filter_mgr_.InvokeRpc(h, aimrt::rpc::ContextRef(ctx), req, rsp)) |
              aimrt::co::Then(
                  [result_callback{std::move(result_callback)}](aimrt::rpc::Status status) {
                    result_callback(status.Code());
                  }));
        });
    RegisterServiceFunc(
        "pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
        nullptr,
        aimrt::GetProtobufMessageTypeSupport<{{rpc_req_name}}>(),
        aimrt::GetProtobufMessageTypeSupport<{{rpc_rsp_name}}>(),
        std::move(service_callback));
  }"""

    t_ccfile_one_service_class: str = r"""
{{service_name}}CoService::{{service_name}}CoService() {
{{ccfile_service_register_func}}
}
"""

    t_ccfile_one_service_one_register_client_func: str = r"""
  if (!(rpc_handle_ref.RegisterClientFunc(
          "pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
          nullptr,
          aimrt::GetProtobufMessageTypeSupport<{{rpc_req_name}}>(),
          aimrt::GetProtobufMessageTypeSupport<{{rpc_rsp_name}}>())))
    return false;"""

    t_ccfile_one_service_register_client_func: str = r"""
bool Register{{service_name}}ClientFunc(aimrt::rpc::RpcHandleRef rpc_handle_ref) {
{{ccfile_service_one_register_client_func}}
  return true;
}"""

    t_ccfile_one_service_sync_proxy_func: str = r"""
aimrt::rpc::Status {{service_name}}SyncProxy::{{rpc_func_name}}(
    aimrt::rpc::ContextRef ctx_ref,
    const {{rpc_req_name}}& req,
    {{rpc_rsp_name}}& rsp) {
  if (ctx_ref) {
    if (ctx_ref.GetSerializationType().empty())
      ctx_ref.SetSerializationType("pb");
  } else {
    ctx_ref = rpc_handle_ref_.NewContextRef();
    ctx_ref.SetSerializationType("pb");
  }

  std::promise<aimrt::rpc::Status> result_promise;

  rpc_handle_ref_.Invoke(
      "pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
      ctx_ref,
      &req,
      &rsp,
      [&result_promise](uint32_t code) {
        result_promise.set_value(aimrt::rpc::Status(code));
      });

  return result_promise.get_future().get();
}"""

    t_ccfile_one_service_async_proxy_func: str = r"""
void {{service_name}}AsyncProxy::{{rpc_func_name}}(
    aimrt::rpc::ContextRef ctx_ref,
    const {{rpc_req_name}}& req,
    {{rpc_rsp_name}}& rsp,
    aimrt::util::Function<void(aimrt::rpc::Status)>&& callback) {
  if (ctx_ref) {
    if (ctx_ref.GetSerializationType().empty()) ctx_ref.SetSerializationType("pb");

    rpc_handle_ref_.Invoke(
        "pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
        ctx_ref,
        &req,
        &rsp,
        [callback{std::move(callback)}](uint32_t code) {
          callback(aimrt::rpc::Status(code));
        });

    return;
  }

  auto ctx_ptr = rpc_handle_ref_.NewContextSharedPtr();
  ctx_ref = aimrt::rpc::ContextRef(ctx_ptr.get());
  ctx_ref.SetSerializationType("pb");

  rpc_handle_ref_.Invoke(
      "pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
      ctx_ref,
      &req,
      &rsp,
      [ctx_ptr, callback{std::move(callback)}](uint32_t code) {
        callback(aimrt::rpc::Status(code));
      });
}"""

    t_ccfile_one_service_future_proxy_func: str = r"""
std::future<aimrt::rpc::Status> {{service_name}}FutureProxy::{{rpc_func_name}}(
    aimrt::rpc::ContextRef ctx_ref,
    const {{rpc_req_name}}& req,
    {{rpc_rsp_name}}& rsp) {
  std::promise<aimrt::rpc::Status> status_promise;
  std::future<aimrt::rpc::Status> status_future = status_promise.get_future();

  if (ctx_ref) {
    if (ctx_ref.GetSerializationType().empty()) ctx_ref.SetSerializationType("pb");

    rpc_handle_ref_.Invoke(
        "pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
        ctx_ref,
        &req,
        &rsp,
        [status_promise{std::move(status_promise)}](uint32_t code) mutable {
          status_promise.set_value(aimrt::rpc::Status(code));
        });

    return status_future;
  }

  auto ctx_ptr = rpc_handle_ref_.NewContextSharedPtr();
  ctx_ref = aimrt::rpc::ContextRef(ctx_ptr.get());
  ctx_ref.SetSerializationType("pb");

  rpc_handle_ref_.Invoke(
      "pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
      ctx_ref,
      &req,
      &rsp,
      [ctx_ptr, status_promise{std::move(status_promise)}](uint32_t code) mutable {
        status_promise.set_value(aimrt::rpc::Status(code));
      });

  return status_future;
}"""

    t_ccfile_one_service_proxy_func: str = r"""
aimrt::co::Task<aimrt::rpc::Status> {{service_name}}CoProxy::{{rpc_func_name}}(
    aimrt::rpc::ContextRef ctx_ref,
    const {{rpc_req_name}}& req,
    {{rpc_rsp_name}}& rsp) {
  const aimrt::rpc::RpcHandle h =
      [rpc_handle_ref{rpc_handle_ref_}](aimrt::rpc::ContextRef ctx_ref, const void* req_ptr, void* rsp_ptr)
      -> aimrt::co::Task<aimrt::rpc::Status> {
    co_return co_await aimrt::co::AsyncWrapper<aimrt::rpc::Status>(
        [rpc_handle_ref, ctx_ref, req_ptr, rsp_ptr](
            aimrt::util::Function<void(aimrt::rpc::Status)>&& callback) {
          rpc_handle_ref.Invoke(
              "pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
              ctx_ref, req_ptr, rsp_ptr,
              [callback{std::move(callback)}](uint32_t code) {
                callback(aimrt::rpc::Status(code));
              });
        });
  };

  if (ctx_ref) {
    if (ctx_ref.GetSerializationType().empty()) ctx_ref.SetSerializationType("pb");
    co_return co_await filter_mgr_.InvokeRpc(h, ctx_ref, static_cast<const void*>(&req), static_cast<void*>(&rsp));
  }

  auto ctx_ptr = rpc_handle_ref_.NewContextSharedPtr();
  ctx_ref = aimrt::rpc::ContextRef(ctx_ptr.get());
  ctx_ref.SetSerializationType("pb");
  co_return co_await filter_mgr_.InvokeRpc(h, ctx_ref, static_cast<const void*>(&req), static_cast<void*>(&rsp));
}"""

    t_ccfile_one_service_sync_proxy_class: str = r"""
{{ccfile_service_sync_proxy_func}}
"""

    t_ccfile_one_service_async_proxy_class: str = r"""
{{ccfile_service_async_proxy_func}}
"""

    t_ccfile_one_service_future_proxy_class: str = r"""
{{ccfile_service_future_proxy_func}}
"""

    t_ccfile_one_service_proxy_class: str = r"""
{{ccfile_service_proxy_func}}
"""

    t_ccfile: str = r"""/**
 * @file {{file_name}}.aimrt_rpc.pb.cc
 * @brief This file was generated by protoc-gen-aimrt_rpc which is a self-defined pb compiler plugin, do not edit it!!!
 */

#include "{{file_name}}.aimrt_rpc.pb.h"

#include "aimrt_module_cpp_interface/co/async_wrapper.h"
#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/start_detached.h"
#include "aimrt_module_cpp_interface/co/then.h"
#include "aimrt_module_protobuf_interface/util/protobuf_type_support.h"

#include <google/protobuf/stubs/stringpiece.h>
#include <google/protobuf/util/json_util.h>

{{namespace_begin}}
{{ccfile_sync_service_class}}
{{ccfile_async_service_class}}
{{ccfile_service_class}}
{{ccfile_service_register_client_func}}
{{ccfile_service_sync_proxy_class}}
{{ccfile_service_async_proxy_class}}
{{ccfile_service_future_proxy_class}}
{{ccfile_service_proxy_class}}
{{namespace_end}}
"""

    @staticmethod
    def gen_name_space_str(ns: str) -> str:
        return ns.replace(".", "::")

    @staticmethod
    def gen_namespace_begin_str(ns: str) -> str:
        namespace_vec: list[str] = ns.split(".")
        result: str = ""
        for itr in namespace_vec:
            result += "namespace " + itr + " {\n"
        return result

    @staticmethod
    def gen_namespace_end_str(ns: str) -> str:
        namespace_vec: list[str] = ns.split(".")
        namespace_vec.reverse()
        result: str = ""
        for itr in namespace_vec:
            result += "}  // namespace " + itr + "\n"
        return result

    def generate(self, request: CodeGeneratorRequest) -> CodeGeneratorResponse:
        """Generate code for the given request"""
        response: CodeGeneratorResponse = CodeGeneratorResponse()
        for proto_file in request.proto_file:
            if proto_file.name not in request.file_to_generate:
                continue

            # Generate code for each file
            file_name: str = proto_file.name
            package_name: str = proto_file.package
            namespace_begin: str = self.gen_namespace_begin_str(package_name)
            namespace_end: str = self.gen_namespace_end_str(package_name)

            cc_file_name: str = file_name.replace('.proto', '.aimrt_rpc.pb.cc')
            h_file_name: str = file_name.replace('.proto', '.aimrt_rpc.pb.h')

            hfile_sync_service_class: str = ""
            hfile_async_service_class: str = ""
            hfile_service_class: str = ""
            hfile_service_register_client_func: str = ""
            hfile_service_sync_proxy_class: str = ""
            hfile_service_async_proxy_class: str = ""
            hfile_service_future_proxy_class: str = ""
            hfile_service_proxy_class: str = ""

            ccfile_sync_service_class: str = ""
            ccfile_async_service_class: str = ""
            ccfile_service_class: str = ""
            ccfile_service_register_client_func: str = ""
            ccfile_service_sync_proxy_class: str = ""
            ccfile_service_async_proxy_class: str = ""
            ccfile_service_future_proxy_class: str = ""
            ccfile_service_proxy_class: str = ""

            for ii in range(0, len(proto_file.service)):
                service: ServiceDescriptorProto = proto_file.service[ii]
                if ii != 0:
                    hfile_sync_service_class += "\n"
                    hfile_async_service_class += "\n"
                    hfile_service_class += "\n"
                    hfile_service_register_client_func += "\n"
                    hfile_service_sync_proxy_class += "\n"
                    hfile_service_async_proxy_class += "\n"
                    hfile_service_future_proxy_class += "\n"
                    hfile_service_proxy_class += "\n"

                    ccfile_sync_service_class += "\n"
                    ccfile_async_service_class += "\n"
                    ccfile_service_class += "\n"
                    ccfile_service_register_client_func += "\n"
                    ccfile_service_sync_proxy_class += "\n"
                    ccfile_service_async_proxy_class += "\n"
                    ccfile_service_future_proxy_class += "\n"
                    ccfile_service_proxy_class += "\n"

                service_name: str = service.name

                hfile_sync_service_func: str = ""
                hfile_async_service_func: str = ""
                hfile_service_func: str = ""
                hfile_service_sync_proxy_func: str = ""
                hfile_service_async_proxy_func: str = ""
                hfile_service_future_proxy_func: str = ""
                hfile_service_proxy_func: str = ""

                ccfile_sync_service_register_func: str = ""
                ccfile_async_service_register_func: str = ""
                ccfile_service_register_func: str = ""
                ccfile_service_one_register_client_func: str = ""
                ccfile_service_sync_proxy_func: str = ""
                ccfile_service_async_proxy_func: str = ""
                ccfile_service_future_proxy_func: str = ""
                ccfile_service_proxy_func: str = ""

                for jj in range(0, len(service.method)):
                    method: MethodDescriptorProto = service.method[jj]

                    if jj != 0:
                        hfile_sync_service_func += "\n"
                        hfile_async_service_func += "\n"
                        hfile_service_func += "\n"
                        hfile_service_sync_proxy_func += "\n"
                        hfile_service_async_proxy_func += "\n"
                        hfile_service_future_proxy_func += "\n"
                        hfile_service_proxy_func += "\n"

                        ccfile_sync_service_register_func += "\n"
                        ccfile_async_service_register_func += "\n"
                        ccfile_service_register_func += "\n"
                        ccfile_service_one_register_client_func += "\n"
                        ccfile_service_sync_proxy_func += "\n"
                        ccfile_service_async_proxy_func += "\n"
                        ccfile_service_future_proxy_func += "\n"
                        ccfile_service_proxy_func += "\n"

                    rpc_func_name = method.name
                    rpc_req_name = self.gen_name_space_str(method.input_type)
                    rpc_rsp_name = self.gen_name_space_str(method.output_type)

                    hfile_sync_service_func += self.t_hfile_one_sync_service_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    hfile_async_service_func += self.t_hfile_one_async_service_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    hfile_service_func += self.t_hfile_one_service_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    hfile_service_sync_proxy_func += self.t_hfile_one_service_sync_proxy_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    hfile_service_async_proxy_func += self.t_hfile_one_service_async_proxy_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    hfile_service_future_proxy_func += self.t_hfile_one_service_future_proxy_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    hfile_service_proxy_func += self.t_hfile_one_service_proxy_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    ccfile_sync_service_register_func += self.t_ccfile_one_sync_service_register_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    ccfile_async_service_register_func += self.t_ccfile_one_async_service_register_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    ccfile_service_register_func += self.t_ccfile_one_service_register_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    ccfile_service_one_register_client_func += self.t_ccfile_one_service_one_register_client_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    ccfile_service_sync_proxy_func += self.t_ccfile_one_service_sync_proxy_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    ccfile_service_async_proxy_func += self.t_ccfile_one_service_async_proxy_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    ccfile_service_future_proxy_func += self.t_ccfile_one_service_future_proxy_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                    ccfile_service_proxy_func += self.t_ccfile_one_service_proxy_func \
                        .replace("{{rpc_req_name}}", rpc_req_name) \
                        .replace("{{rpc_rsp_name}}", rpc_rsp_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)

                hfile_sync_service_class += self.t_hfile_one_sync_service_class \
                    .replace("{{hfile_sync_service_func}}", hfile_sync_service_func) \
                    .replace("{{service_name}}", service_name)

                hfile_async_service_class += self.t_hfile_one_async_service_class \
                    .replace("{{hfile_async_service_func}}", hfile_async_service_func) \
                    .replace("{{service_name}}", service_name)

                hfile_service_class += self.t_hfile_one_service_class \
                    .replace("{{hfile_service_func}}", hfile_service_func) \
                    .replace("{{service_name}}", service_name)

                hfile_service_register_client_func += self.t_hfile_one_service_register_client_func \
                    .replace("{{service_name}}", service_name)

                hfile_service_sync_proxy_class += self.t_hfile_one_service_sync_proxy_class \
                    .replace("{{hfile_service_sync_proxy_func}}", hfile_service_sync_proxy_func) \
                    .replace("{{service_name}}", service_name)

                hfile_service_async_proxy_class += self.t_hfile_one_service_async_proxy_class \
                    .replace("{{hfile_service_async_proxy_func}}", hfile_service_async_proxy_func) \
                    .replace("{{service_name}}", service_name)

                hfile_service_future_proxy_class += self.t_hfile_one_service_future_proxy_class \
                    .replace("{{hfile_service_future_proxy_func}}", hfile_service_future_proxy_func) \
                    .replace("{{service_name}}", service_name)

                hfile_service_proxy_class += self.t_hfile_one_service_proxy_class \
                    .replace("{{hfile_service_proxy_func}}", hfile_service_proxy_func) \
                    .replace("{{service_name}}", service_name)

                ccfile_sync_service_class += self.t_ccfile_one_sync_service_class \
                    .replace("{{ccfile_sync_service_register_func}}", ccfile_sync_service_register_func) \
                    .replace("{{service_name}}", service_name)

                ccfile_async_service_class += self.t_ccfile_one_async_service_class \
                    .replace("{{ccfile_async_service_register_func}}", ccfile_async_service_register_func) \
                    .replace("{{service_name}}", service_name)

                ccfile_service_class += self.t_ccfile_one_service_class \
                    .replace("{{ccfile_service_register_func}}", ccfile_service_register_func) \
                    .replace("{{service_name}}", service_name)

                ccfile_service_register_client_func += self.t_ccfile_one_service_register_client_func \
                    .replace("{{ccfile_service_one_register_client_func}}", ccfile_service_one_register_client_func) \
                    .replace("{{service_name}}", service_name)

                ccfile_service_sync_proxy_class += self.t_ccfile_one_service_sync_proxy_class \
                    .replace("{{ccfile_service_sync_proxy_func}}", ccfile_service_sync_proxy_func) \
                    .replace("{{service_name}}", service_name)

                ccfile_service_async_proxy_class += self.t_ccfile_one_service_async_proxy_class \
                    .replace("{{ccfile_service_async_proxy_func}}", ccfile_service_async_proxy_func) \
                    .replace("{{service_name}}", service_name)

                ccfile_service_future_proxy_class += self.t_ccfile_one_service_future_proxy_class \
                    .replace("{{ccfile_service_future_proxy_func}}", ccfile_service_future_proxy_func) \
                    .replace("{{service_name}}", service_name)

                ccfile_service_proxy_class += self.t_ccfile_one_service_proxy_class \
                    .replace("{{ccfile_service_proxy_func}}", ccfile_service_proxy_func) \
                    .replace("{{service_name}}", service_name)

            # hfile
            hfile: CodeGeneratorResponse.File = CodeGeneratorResponse.File()
            hfile.name = h_file_name
            hfile.content = self.t_hfile \
                .replace("{{hfile_sync_service_class}}", hfile_sync_service_class) \
                .replace("{{hfile_async_service_class}}", hfile_async_service_class) \
                .replace("{{hfile_service_class}}", hfile_service_class) \
                .replace("{{hfile_service_register_client_func}}", hfile_service_register_client_func) \
                .replace("{{hfile_service_sync_proxy_class}}", hfile_service_sync_proxy_class) \
                .replace("{{hfile_service_async_proxy_class}}", hfile_service_async_proxy_class) \
                .replace("{{hfile_service_future_proxy_class}}", hfile_service_future_proxy_class) \
                .replace("{{hfile_service_proxy_class}}", hfile_service_proxy_class) \
                .replace("{{file_name}}", file_name.replace('.proto', '')) \
                .replace("{{namespace_begin}}", namespace_begin) \
                .replace("{{namespace_end}}", namespace_end) \
                .replace("{{package_name}}", package_name)
            response.file.append(hfile)

            # ccfile
            ccfile: CodeGeneratorResponse.File = CodeGeneratorResponse.File()
            ccfile.name = cc_file_name
            ccfile.content = self.t_ccfile \
                .replace("{{ccfile_sync_service_class}}", ccfile_sync_service_class) \
                .replace("{{ccfile_async_service_class}}", ccfile_async_service_class) \
                .replace("{{ccfile_service_class}}", ccfile_service_class) \
                .replace("{{ccfile_service_register_client_func}}", ccfile_service_register_client_func) \
                .replace("{{ccfile_service_sync_proxy_class}}", ccfile_service_sync_proxy_class) \
                .replace("{{ccfile_service_async_proxy_class}}", ccfile_service_async_proxy_class) \
                .replace("{{ccfile_service_future_proxy_class}}", ccfile_service_future_proxy_class) \
                .replace("{{ccfile_service_proxy_class}}", ccfile_service_proxy_class) \
                .replace("{{file_name}}", file_name.replace('.proto', '')) \
                .replace("{{namespace_begin}}", namespace_begin) \
                .replace("{{namespace_end}}", namespace_end) \
                .replace("{{package_name}}", package_name)
            response.file.append(ccfile)

        return response


if __name__ == '__main__':
    # Load the request from stdin
    request: CodeGeneratorRequest = CodeGeneratorRequest.FromString(sys.stdin.buffer.read())

    aimrt_code_generator: AimRTCodeGenerator = AimRTCodeGenerator()

    response: CodeGeneratorResponse = aimrt_code_generator.generate(request)

    # Serialize response and write to stdout
    sys.stdout.buffer.write(response.SerializeToString())
