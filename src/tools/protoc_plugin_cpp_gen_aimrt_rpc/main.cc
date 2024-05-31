#include <string>
#include <vector>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/compiler/plugin.pb.h>
#include <google/protobuf/descriptor.h>

std::string& ReplaceString(std::string& source, const std::string& replace_what,
                           const std::string& replace_with_what) {
  std::string::size_type pos = 0;
  while (true) {
    pos = source.find(replace_what, pos);
    if (pos == std::string::npos) break;
    source.replace(pos, replace_what.size(), replace_with_what);
    pos += replace_with_what.size();
  }

  return source;
}

std::vector<std::string> SplitToVec(const std::string& source,
                                    const std::string& sep, bool clear = true) {
  std::vector<std::string> result;
  if (source.empty() || sep.empty()) return result;

  size_t pos_end, pos_start = 0;
  do {
    pos_end = source.find(sep, pos_start);
    if (pos_end == std::string::npos) pos_end = source.length();

    const std::string& sub_str = source.substr(pos_start, pos_end - pos_start);
    if (!(clear && sub_str.empty())) {
      result.emplace_back(sub_str);
    }

    pos_start = pos_end + sep.size();
  } while (pos_end < source.length());

  return result;
}

std::string ReplaceMultiString(
    std::string_view source, const std::map<std::string, std::string>& replace_info_map) {
  std::string result(source);

  for (auto& itr : replace_info_map)
    ReplaceString(result, itr.first, itr.second);

  return result;
}

class AimRTCodeGenerator final : public google::protobuf::compiler::CodeGenerator {
 public:
  AimRTCodeGenerator() = default;
  ~AimRTCodeGenerator() override = default;

  uint64_t GetSupportedFeatures() const override {
    return FEATURE_PROTO3_OPTIONAL;
  }

  constexpr static std::string_view t_hfile_one_sync_service_func = R"str(
  virtual aimrt::rpc::Status {{rpc_func_name}}(
      aimrt::rpc::ContextRef ctx_ref,
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp) {
    return aimrt::rpc::Status(AIMRT_RPC_STATUS_SVR_NOT_IMPLEMENTED);
  })str";

  constexpr static std::string_view t_hfile_one_sync_service_class = R"str(
class {{service_name}}SyncService : public aimrt::rpc::ServiceBase {
 public:
  {{service_name}}SyncService();
  ~{{service_name}}SyncService() override = default;
{{hfile_sync_service_func}}
};)str";

  constexpr static std::string_view t_hfile_one_async_service_func = R"str(
  virtual void {{rpc_func_name}}(
      aimrt::rpc::ContextRef ctx_ref,
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp,
      aimrt::util::Function<void(aimrt::rpc::Status)>&& callback) {
    callback(aimrt::rpc::Status(AIMRT_RPC_STATUS_SVR_NOT_IMPLEMENTED));
  })str";

  constexpr static std::string_view t_hfile_one_async_service_class = R"str(
class {{service_name}}AsyncService : public aimrt::rpc::ServiceBase {
 public:
  {{service_name}}AsyncService();
  ~{{service_name}}AsyncService() override = default;
{{hfile_async_service_func}}
};)str";

  constexpr static std::string_view t_hfile_one_service_func = R"str(
  virtual aimrt::co::Task<aimrt::rpc::Status> {{rpc_func_name}}(
      aimrt::rpc::ContextRef ctx_ref,
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp) {
    co_return aimrt::rpc::Status(AIMRT_RPC_STATUS_SVR_NOT_IMPLEMENTED);
  })str";

  constexpr static std::string_view t_hfile_one_service_class = R"str(
class {{service_name}}CoService : public aimrt::rpc::CoServiceBase {
 public:
  {{service_name}}CoService();
  ~{{service_name}}CoService() override = default;
{{hfile_service_func}}
};

using {{service_name}} [[deprecated("Using {{service_name}}CoService.")]] = {{service_name}}CoService;)str";

  constexpr static std::string_view t_hfile_one_service_register_client_func = R"str(
bool Register{{service_name}}ClientFunc(aimrt::rpc::RpcHandleRef rpc_handle_ref);)str";

  constexpr static std::string_view t_hfile_one_service_sync_proxy_func = R"str(
  aimrt::rpc::Status {{rpc_func_name}}(
      aimrt::rpc::ContextRef ctx_ref,
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp);

  aimrt::rpc::Status {{rpc_func_name}}(
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp) {
    return {{rpc_func_name}}(aimrt::rpc::ContextRef(), req, rsp);
  })str";

  constexpr static std::string_view t_hfile_one_service_sync_proxy_class = R"str(
class {{service_name}}SyncProxy : public aimrt::rpc::ProxyBase {
 public:
  explicit {{service_name}}SyncProxy(aimrt::rpc::RpcHandleRef rpc_handle_ref)
      : aimrt::rpc::ProxyBase(rpc_handle_ref) {}
  ~{{service_name}}SyncProxy() = default;

  static bool RegisterClientFunc(aimrt::rpc::RpcHandleRef rpc_handle_ref) {
    return Register{{service_name}}ClientFunc(rpc_handle_ref);
  }
{{hfile_service_sync_proxy_func}}
};)str";

  constexpr static std::string_view t_hfile_one_service_async_proxy_func = R"str(
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
  })str";

  constexpr static std::string_view t_hfile_one_service_async_proxy_class = R"str(
class {{service_name}}AsyncProxy : public aimrt::rpc::ProxyBase {
 public:
  explicit {{service_name}}AsyncProxy(aimrt::rpc::RpcHandleRef rpc_handle_ref)
      : aimrt::rpc::ProxyBase(rpc_handle_ref) {}
  ~{{service_name}}AsyncProxy() = default;

  static bool RegisterClientFunc(aimrt::rpc::RpcHandleRef rpc_handle_ref) {
    return Register{{service_name}}ClientFunc(rpc_handle_ref);
  }
{{hfile_service_async_proxy_func}}
};)str";

  constexpr static std::string_view t_hfile_one_service_future_proxy_func = R"str(
  std::future<aimrt::rpc::Status> {{rpc_func_name}}(
      aimrt::rpc::ContextRef ctx_ref,
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp);

  std::future<aimrt::rpc::Status> {{rpc_func_name}}(
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp) {
    return {{rpc_func_name}}(aimrt::rpc::ContextRef(), req, rsp);
  })str";

  constexpr static std::string_view t_hfile_one_service_future_proxy_class = R"str(
class {{service_name}}FutureProxy : public aimrt::rpc::ProxyBase {
 public:
  explicit {{service_name}}FutureProxy(aimrt::rpc::RpcHandleRef rpc_handle_ref)
      : aimrt::rpc::ProxyBase(rpc_handle_ref) {}
  ~{{service_name}}FutureProxy() = default;

  static bool RegisterClientFunc(aimrt::rpc::RpcHandleRef rpc_handle_ref) {
    return Register{{service_name}}ClientFunc(rpc_handle_ref);
  }
{{hfile_service_future_proxy_func}}
};)str";

  constexpr static std::string_view t_hfile_one_service_proxy_func = R"str(
  aimrt::co::Task<aimrt::rpc::Status> {{rpc_func_name}}(
      aimrt::rpc::ContextRef ctx_ref,
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp);

  aimrt::co::Task<aimrt::rpc::Status> {{rpc_func_name}}(
      const {{rpc_req_name}}& req,
      {{rpc_rsp_name}}& rsp) {
    return {{rpc_func_name}}(aimrt::rpc::ContextRef(), req, rsp);
  })str";

  constexpr static std::string_view t_hfile_one_service_proxy_class = R"str(
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

using {{service_name}}Proxy [[deprecated("Using {{service_name}}CoProxy.")]] = {{service_name}}CoProxy;)str";

  constexpr static std::string_view t_hfile = R"str(/**
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
)str";

  constexpr static std::string_view t_ccfile_one_sync_service_register_func = R"str(
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
  })str";

  constexpr static std::string_view t_ccfile_one_sync_service_class = R"str(
{{service_name}}SyncService::{{service_name}}SyncService() {
{{ccfile_sync_service_register_func}}
}
)str";

  constexpr static std::string_view t_ccfile_one_async_service_register_func = R"str(
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
  })str";

  constexpr static std::string_view t_ccfile_one_async_service_class = R"str(
{{service_name}}AsyncService::{{service_name}}AsyncService() {
{{ccfile_async_service_register_func}}
}
)str";

  constexpr static std::string_view t_ccfile_one_service_register_func = R"str(
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
  })str";

  constexpr static std::string_view t_ccfile_one_service_class = R"str(
{{service_name}}CoService::{{service_name}}CoService() {
{{ccfile_service_register_func}}
}
)str";

  constexpr static std::string_view t_ccfile_one_service_one_register_client_func = R"str(
  if (!(rpc_handle_ref.RegisterClientFunc(
          "pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
          nullptr,
          aimrt::GetProtobufMessageTypeSupport<{{rpc_req_name}}>(),
          aimrt::GetProtobufMessageTypeSupport<{{rpc_rsp_name}}>())))
    return false;)str";

  constexpr static std::string_view t_ccfile_one_service_register_client_func = R"str(
bool Register{{service_name}}ClientFunc(aimrt::rpc::RpcHandleRef rpc_handle_ref) {
{{ccfile_service_one_register_client_func}}
  return true;
})str";

  constexpr static std::string_view t_ccfile_one_service_sync_proxy_func = R"str(
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
})str";

  constexpr static std::string_view t_ccfile_one_service_async_proxy_func = R"str(
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
})str";

  constexpr static std::string_view t_ccfile_one_service_future_proxy_func = R"str(
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
})str";

  constexpr static std::string_view t_ccfile_one_service_proxy_func = R"str(
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
})str";

  constexpr static std::string_view t_ccfile_one_service_sync_proxy_class = R"str(
{{ccfile_service_sync_proxy_func}}
)str";

  constexpr static std::string_view t_ccfile_one_service_async_proxy_class = R"str(
{{ccfile_service_async_proxy_func}}
)str";

  constexpr static std::string_view t_ccfile_one_service_future_proxy_class = R"str(
{{ccfile_service_future_proxy_func}}
)str";

  constexpr static std::string_view t_ccfile_one_service_proxy_class = R"str(
{{ccfile_service_proxy_func}}
)str";

  constexpr static std::string_view t_ccfile = R"str(/**
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
)str";

  static std::string ProtoFileBaseName(const std::string& full_name) {
    return full_name.substr(0, full_name.rfind("."));
  }

  static std::string GenNamespaceStr(const std::string& ns) {
    std::string result = ns;
    return ReplaceString(result, ".", "::");
  }

  static std::string GenNamespaceBeginStr(const std::string& ns) {
    std::vector<std::string> namespace_vec = SplitToVec(ns, ".");
    std::string result;
    for (const auto& itr : namespace_vec) {
      result += ("namespace " + itr + " {\n");
    }
    return result;
  }

  static std::string GenNamespaceEndStr(const std::string& ns) {
    std::vector<std::string> namespace_vec = SplitToVec(ns, ".");
    std::reverse(namespace_vec.begin(), namespace_vec.end());
    std::string result;
    for (const auto& itr : namespace_vec) {
      result += ("}  // namespace " + itr + "\n");
    }
    return result;
  }

  static void WriteToFile(google::protobuf::compiler::GeneratorContext* context,
                          const std::string& file_name,
                          const std::string& file_context) {
    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(context->Open(file_name));
    google::protobuf::io::CodedOutputStream coded_out(output.get());
    coded_out.WriteRaw(file_context.data(), file_context.size());
  }

  bool Generate(const google::protobuf::FileDescriptor* file,
                const std::string& parameter,
                google::protobuf::compiler::GeneratorContext* context,
                std::string* error) const override {
    const std::string& file_name = ProtoFileBaseName(file->name());
    const std::string& package_name = file->package();
    const std::string& namespace_begin = GenNamespaceBeginStr(file->package());
    const std::string& namespace_end = GenNamespaceEndStr(file->package());

    // file
    std::string hfile_sync_service_class;
    std::string hfile_async_service_class;
    std::string hfile_service_class;
    std::string hfile_service_register_client_func;
    std::string hfile_service_sync_proxy_class;
    std::string hfile_service_async_proxy_class;
    std::string hfile_service_future_proxy_class;
    std::string hfile_service_proxy_class;

    std::string ccfile_sync_service_class;
    std::string ccfile_async_service_class;
    std::string ccfile_service_class;
    std::string ccfile_service_register_client_func;
    std::string ccfile_service_sync_proxy_class;
    std::string ccfile_service_async_proxy_class;
    std::string ccfile_service_future_proxy_class;
    std::string ccfile_service_proxy_class;

    for (int ii = 0; ii < file->service_count(); ++ii) {
      // service
      auto service = file->service(ii);

      if (ii != 0) {
        hfile_sync_service_class += "\n";
        hfile_async_service_class += "\n";
        hfile_service_class += "\n";
        hfile_service_register_client_func += "\n";
        hfile_service_sync_proxy_class += "\n";
        hfile_service_async_proxy_class += "\n";
        hfile_service_future_proxy_class += "\n";
        hfile_service_proxy_class += "\n";

        ccfile_sync_service_class += "\n";
        ccfile_async_service_class += "\n";
        ccfile_service_class += "\n";
        ccfile_service_register_client_func += "\n";
        ccfile_service_sync_proxy_class += "\n";
        ccfile_service_async_proxy_class += "\n";
        ccfile_service_future_proxy_class += "\n";
        ccfile_service_proxy_class += "\n";
      }

      const std::string& service_name = service->name();

      std::string hfile_sync_service_func;
      std::string hfile_async_service_func;
      std::string hfile_service_func;
      std::string hfile_service_sync_proxy_func;
      std::string hfile_service_async_proxy_func;
      std::string hfile_service_future_proxy_func;
      std::string hfile_service_proxy_func;

      std::string ccfile_sync_service_register_func;
      std::string ccfile_async_service_register_func;
      std::string ccfile_service_register_func;
      std::string ccfile_service_one_register_client_func;
      std::string ccfile_service_sync_proxy_func;
      std::string ccfile_service_async_proxy_func;
      std::string ccfile_service_future_proxy_func;
      std::string ccfile_service_proxy_func;

      for (int jj = 0; jj < service->method_count(); ++jj) {
        // method
        auto method = service->method(jj);

        if (jj != 0) {
          hfile_sync_service_func += "\n";
          hfile_async_service_func += "\n";
          hfile_service_func += "\n";
          hfile_service_sync_proxy_func += "\n";
          hfile_service_async_proxy_func += "\n";
          hfile_service_future_proxy_func += "\n";
          hfile_service_proxy_func += "\n";

          ccfile_sync_service_register_func += "\n";
          ccfile_async_service_register_func += "\n";
          ccfile_service_register_func += "\n";
          ccfile_service_one_register_client_func += "\n";
          ccfile_service_sync_proxy_func += "\n";
          ccfile_service_async_proxy_func += "\n";
          ccfile_service_future_proxy_func += "\n";
          ccfile_service_proxy_func += "\n";
        }

        const std::string& rpc_func_name = method->name();
        const std::string& rpc_req_name = "::" + GenNamespaceStr(method->input_type()->full_name());
        const std::string& rpc_rsp_name = "::" + GenNamespaceStr(method->output_type()->full_name());

        std::map<std::string, std::string> replace_info_map{
            {"{{rpc_req_name}}", rpc_req_name},
            {"{{rpc_rsp_name}}", rpc_rsp_name},
            {"{{rpc_func_name}}", rpc_func_name}};

        hfile_sync_service_func += ReplaceMultiString(t_hfile_one_sync_service_func, replace_info_map);
        hfile_async_service_func += ReplaceMultiString(t_hfile_one_async_service_func, replace_info_map);
        hfile_service_func += ReplaceMultiString(t_hfile_one_service_func, replace_info_map);
        hfile_service_sync_proxy_func += ReplaceMultiString(t_hfile_one_service_sync_proxy_func, replace_info_map);
        hfile_service_async_proxy_func += ReplaceMultiString(t_hfile_one_service_async_proxy_func, replace_info_map);
        hfile_service_future_proxy_func += ReplaceMultiString(t_hfile_one_service_future_proxy_func, replace_info_map);
        hfile_service_proxy_func += ReplaceMultiString(t_hfile_one_service_proxy_func, replace_info_map);

        ccfile_sync_service_register_func += ReplaceMultiString(t_ccfile_one_sync_service_register_func, replace_info_map);
        ccfile_async_service_register_func += ReplaceMultiString(t_ccfile_one_async_service_register_func, replace_info_map);
        ccfile_service_register_func += ReplaceMultiString(t_ccfile_one_service_register_func, replace_info_map);
        ccfile_service_one_register_client_func += ReplaceMultiString(t_ccfile_one_service_one_register_client_func, replace_info_map);
        ccfile_service_sync_proxy_func += ReplaceMultiString(t_ccfile_one_service_sync_proxy_func, replace_info_map);
        ccfile_service_async_proxy_func += ReplaceMultiString(t_ccfile_one_service_async_proxy_func, replace_info_map);
        ccfile_service_future_proxy_func += ReplaceMultiString(t_ccfile_one_service_future_proxy_func, replace_info_map);
        ccfile_service_proxy_func += ReplaceMultiString(t_ccfile_one_service_proxy_func, replace_info_map);
      }

      hfile_sync_service_class += ReplaceMultiString(
          t_hfile_one_sync_service_class,
          {{"{{hfile_sync_service_func}}", hfile_sync_service_func},
           {"{{service_name}}", service_name}});

      hfile_async_service_class += ReplaceMultiString(
          t_hfile_one_async_service_class,
          {{"{{hfile_async_service_func}}", hfile_async_service_func},
           {"{{service_name}}", service_name}});

      hfile_service_class += ReplaceMultiString(
          t_hfile_one_service_class,
          {{"{{hfile_service_func}}", hfile_service_func},
           {"{{service_name}}", service_name}});

      hfile_service_register_client_func += ReplaceMultiString(
          t_hfile_one_service_register_client_func,
          {{"{{service_name}}", service_name}});

      hfile_service_sync_proxy_class += ReplaceMultiString(
          t_hfile_one_service_sync_proxy_class,
          {{"{{hfile_service_sync_proxy_func}}", hfile_service_sync_proxy_func},
           {"{{service_name}}", service_name}});

      hfile_service_async_proxy_class += ReplaceMultiString(
          t_hfile_one_service_async_proxy_class,
          {{"{{hfile_service_async_proxy_func}}", hfile_service_async_proxy_func},
           {"{{service_name}}", service_name}});

      hfile_service_future_proxy_class += ReplaceMultiString(
          t_hfile_one_service_future_proxy_class,
          {{"{{hfile_service_future_proxy_func}}", hfile_service_future_proxy_func},
           {"{{service_name}}", service_name}});

      hfile_service_proxy_class += ReplaceMultiString(
          t_hfile_one_service_proxy_class,
          {{"{{hfile_service_proxy_func}}", hfile_service_proxy_func},
           {"{{service_name}}", service_name}});

      ccfile_sync_service_class += ReplaceMultiString(
          t_ccfile_one_sync_service_class,
          {{"{{ccfile_sync_service_register_func}}", ccfile_sync_service_register_func},
           {"{{service_name}}", service_name}});

      ccfile_async_service_class += ReplaceMultiString(
          t_ccfile_one_async_service_class,
          {{"{{ccfile_async_service_register_func}}", ccfile_async_service_register_func},
           {"{{service_name}}", service_name}});

      ccfile_service_class += ReplaceMultiString(
          t_ccfile_one_service_class,
          {{"{{ccfile_service_register_func}}", ccfile_service_register_func},
           {"{{service_name}}", service_name}});

      ccfile_service_register_client_func += ReplaceMultiString(
          t_ccfile_one_service_register_client_func,
          {{"{{ccfile_service_one_register_client_func}}", ccfile_service_one_register_client_func},
           {"{{service_name}}", service_name}});

      ccfile_service_sync_proxy_class += ReplaceMultiString(
          t_ccfile_one_service_sync_proxy_class,
          {{"{{ccfile_service_sync_proxy_func}}", ccfile_service_sync_proxy_func},
           {"{{service_name}}", service_name}});

      ccfile_service_async_proxy_class += ReplaceMultiString(
          t_ccfile_one_service_async_proxy_class,
          {{"{{ccfile_service_async_proxy_func}}", ccfile_service_async_proxy_func},
           {"{{service_name}}", service_name}});

      ccfile_service_future_proxy_class += ReplaceMultiString(
          t_ccfile_one_service_future_proxy_class,
          {{"{{ccfile_service_future_proxy_func}}", ccfile_service_future_proxy_func},
           {"{{service_name}}", service_name}});

      ccfile_service_proxy_class += ReplaceMultiString(
          t_ccfile_one_service_proxy_class,
          {{"{{ccfile_service_proxy_func}}", ccfile_service_proxy_func},
           {"{{service_name}}", service_name}});
    }

    // hfile
    std::string hfile = std::string(t_hfile);
    ReplaceString(hfile, "{{hfile_sync_service_class}}", hfile_sync_service_class);
    ReplaceString(hfile, "{{hfile_async_service_class}}", hfile_async_service_class);
    ReplaceString(hfile, "{{hfile_service_class}}", hfile_service_class);
    ReplaceString(hfile, "{{hfile_service_register_client_func}}", hfile_service_register_client_func);
    ReplaceString(hfile, "{{hfile_service_sync_proxy_class}}", hfile_service_sync_proxy_class);
    ReplaceString(hfile, "{{hfile_service_async_proxy_class}}", hfile_service_async_proxy_class);
    ReplaceString(hfile, "{{hfile_service_future_proxy_class}}", hfile_service_future_proxy_class);
    ReplaceString(hfile, "{{hfile_service_proxy_class}}", hfile_service_proxy_class);
    ReplaceString(hfile, "{{file_name}}", file_name);
    ReplaceString(hfile, "{{namespace_begin}}", namespace_begin);
    ReplaceString(hfile, "{{namespace_end}}", namespace_end);
    ReplaceString(hfile, "{{package_name}}", package_name);
    WriteToFile(context, file_name + ".aimrt_rpc.pb.h", hfile);

    // ccfile
    std::string ccfile = std::string(t_ccfile);
    ReplaceString(ccfile, "{{ccfile_sync_service_class}}", ccfile_sync_service_class);
    ReplaceString(ccfile, "{{ccfile_async_service_class}}", ccfile_async_service_class);
    ReplaceString(ccfile, "{{ccfile_service_class}}", ccfile_service_class);
    ReplaceString(ccfile, "{{ccfile_service_register_client_func}}", ccfile_service_register_client_func);
    ReplaceString(ccfile, "{{ccfile_service_sync_proxy_class}}", ccfile_service_sync_proxy_class);
    ReplaceString(ccfile, "{{ccfile_service_async_proxy_class}}", ccfile_service_async_proxy_class);
    ReplaceString(ccfile, "{{ccfile_service_future_proxy_class}}", ccfile_service_future_proxy_class);
    ReplaceString(ccfile, "{{ccfile_service_proxy_class}}", ccfile_service_proxy_class);
    ReplaceString(ccfile, "{{file_name}}", file_name);
    ReplaceString(ccfile, "{{namespace_begin}}", namespace_begin);
    ReplaceString(ccfile, "{{namespace_end}}", namespace_end);
    ReplaceString(ccfile, "{{package_name}}", package_name);
    WriteToFile(context, file_name + ".aimrt_rpc.pb.cc", ccfile);

    return true;
  }
};

int main(int argc, char* argv[]) {
  AimRTCodeGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
