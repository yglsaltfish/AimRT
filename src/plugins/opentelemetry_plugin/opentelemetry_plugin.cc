#include "opentelemetry_plugin/opentelemetry_plugin.h"

#include "core/aimrt_core.h"
#include "opentelemetry_plugin/context_carrier.h"
#include "opentelemetry_plugin/global.h"
#include "opentelemetry_plugin/util.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::opentelemetry_plugin::OpenTelemetryPlugin::Options> {
  using Options = aimrt::plugins::opentelemetry_plugin::OpenTelemetryPlugin::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["node_name"] = rhs.node_name;
    node["otlp_http_exporter_url"] = rhs.otlp_http_exporter_url;

    node["attributes"] = YAML::Node();
    for (const auto& attribute : rhs.attributes) {
      Node attribute_node;
      attribute_node["key"] = attribute.key;
      attribute_node["val"] = attribute.val;
      node["attributes"].push_back(attribute_node);
    }

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    rhs.node_name = node["node_name"].as<std::string>();
    rhs.otlp_http_exporter_url = node["otlp_http_exporter_url"].as<std::string>();

    for (auto& attribute_node : node["attributes"]) {
      auto attribute = Options::Attribute{
          .key = attribute_node["key"].as<std::string>(),
          .val = attribute_node["val"].as<std::string>(),
      };
      rhs.attributes.emplace_back(std::move(attribute));
    }

    return true;
  }
};
}  // namespace YAML

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace resource = opentelemetry::sdk::resource;
namespace otlp = opentelemetry::exporter::otlp;

namespace aimrt::plugins::opentelemetry_plugin {

bool OpenTelemetryPlugin::Initialize(runtime::core::AimRTCore* core_ptr) noexcept {
  try {
    core_ptr_ = core_ptr;

    YAML::Node plugin_options_node = core_ptr_->GetPluginManager().GetPluginOptionsNode(Name());

    if (plugin_options_node && !plugin_options_node.IsNull()) {
      options_ = plugin_options_node.as<Options>();
    }

    init_flag_ = true;

    AIMRT_CHECK_ERROR_THROW(!options_.node_name.empty(), "node name is empty!");

    // init opentelemetry
    auto resource_attributes = resource::ResourceAttributes{
        {"service.name", options_.node_name}};
    for (auto& itr : options_.attributes) {
      resource_attributes.SetAttribute(itr.key, itr.val);
    }
    auto resource = resource::Resource::Create(resource_attributes);

    otlp::OtlpHttpExporterOptions opts;
    opts.url = options_.otlp_http_exporter_url;
    auto exporter = otlp::OtlpHttpExporterFactory::Create(opts);

    trace_sdk::BatchSpanProcessorOptions bspOpts{};
    auto processor = trace_sdk::BatchSpanProcessorFactory::Create(std::move(exporter), bspOpts);

    provider_ = trace_sdk::TracerProviderFactory::Create(std::move(processor), resource);

    propagator_ = std::make_shared<trace_api::propagation::HttpTraceContext>();

    // register hook
    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PostInitLog,
                                [this] { SetPluginLogger(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreInitChannel,
                                [this] { RegisterChannelFilter(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreInitRpc,
                                [this] { RegisterRpcFilter(); });

    plugin_options_node = options_;
    return true;
  } catch (const std::exception& e) {
    AIMRT_ERROR("Initialize failed, {}", e.what());
  }

  return false;
}

void OpenTelemetryPlugin::Shutdown() noexcept {
  try {
    if (!init_flag_) return;

    provider_.reset();
    propagator_.reset();

  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
  }
}

void OpenTelemetryPlugin::SetPluginLogger() {
  SetLogger(aimrt::logger::LoggerRef(
      core_ptr_->GetLoggerManager().GetLoggerProxy().NativeHandle()));
}

void OpenTelemetryPlugin::RegisterChannelFilter() {
  auto& channel_manager = core_ptr_->GetChannelManager();

  channel_manager.SetPassedContextMetaKeys(
      {std::string(kCtxKeyStartNewTrace),
       std::string(kCtxKeyTraceParent),
       std::string(kCtxKeyTraceState)});

  channel_manager.RegisterPublishFilter(
      "otp_trace",
      [this](const aimrt::runtime::core::channel::FrameworkFilterData& filter_data,
             aimrt::channel::ContextRef ctx_ref,
             const void* msg,
             aimrt::runtime::core::channel::FrameworkAsyncChannelHandle&& h) {
        // 如果context强制设置了start_new_trace，或者上层传递了span，则新启动一个span
        std::string start_new_trace_meta_val(ctx_ref.GetMetaValue(kCtxKeyStartNewTrace));
        bool start_new_trace = (common::util::StrToLower(start_new_trace_meta_val) == "true");

        auto tracer = provider_->GetTracer(options_.node_name);
        ContextCarrier carrier(ctx_ref);

        // 解压传进来的context，得到父span
        trace_api::StartSpanOptions op{
            .kind = trace_api::SpanKind::kProducer,
        };

        opentelemetry::context::Context input_ot_ctx;
        auto extract_ctx = propagator_->Extract(carrier, input_ot_ctx);

        auto extract_ctx_val = extract_ctx.GetValue(trace_api::kSpanKey);
        if (!::opentelemetry::nostd::holds_alternative<::opentelemetry::nostd::monostate>(extract_ctx_val)) {
          auto parent_span =
              ::opentelemetry::nostd::get<::opentelemetry::nostd::shared_ptr<::opentelemetry::trace::Span>>(extract_ctx_val);
          op.parent = parent_span->GetContext();
          start_new_trace = true;
        }

        // 不需要启动一个新trace
        if (!start_new_trace) {
          h(filter_data, ctx_ref, msg);
          return;
        }

        // 需要启动一个新trace
        std::string span_name =
            std::string(ctx_ref.GetMetaValue(AIMRT_CHANNEL_CONTEXT_TOPIC_NAME)) + "/" +
            std::string(filter_data.msg_type);
        auto span = tracer->StartSpan(ToNoStdStringView(span_name), op);

        // 将当前span的context打包
        opentelemetry::context::Context output_ot_ctx(trace_api::kSpanKey, span);
        propagator_->Inject(carrier, output_ot_ctx);

        // 添加context中的属性
        auto keys = ctx_ref.GetMetaKeys();
        for (auto& itr : keys) {
          span->SetAttribute(ToNoStdStringView(itr), ToNoStdStringView(ctx_ref.GetMetaValue(itr)));
        }

        h(filter_data, ctx_ref, msg);

        // 序列化包成json
        auto s = filter_data.type_support_ref.SimpleSerialize("json", msg);
        if (!s.empty()) span->SetAttribute("msg_data", s);

        span->End();
      });

  channel_manager.RegisterSubscribeFilter(
      "otp_trace",
      [this](const aimrt::runtime::core::channel::FrameworkFilterData& filter_data,
             aimrt::channel::ContextRef ctx_ref,
             const void* msg,
             aimrt::runtime::core::channel::FrameworkAsyncChannelHandle&& h) {
        // 如果context强制设置了start_new_trace，或者上层传递了span，则新启动一个span
        std::string start_new_trace_meta_val(ctx_ref.GetMetaValue(kCtxKeyStartNewTrace));
        bool start_new_trace = (common::util::StrToLower(start_new_trace_meta_val) == "true");

        auto tracer = provider_->GetTracer(options_.node_name);
        ContextCarrier carrier(ctx_ref);

        // 解压传进来的context，得到父span
        trace_api::StartSpanOptions op{
            .kind = trace_api::SpanKind::kConsumer,
        };

        opentelemetry::context::Context input_ot_ctx;
        auto extract_ctx = propagator_->Extract(carrier, input_ot_ctx);

        auto extract_ctx_val = extract_ctx.GetValue(trace_api::kSpanKey);
        if (!::opentelemetry::nostd::holds_alternative<::opentelemetry::nostd::monostate>(extract_ctx_val)) {
          auto parent_span =
              ::opentelemetry::nostd::get<::opentelemetry::nostd::shared_ptr<::opentelemetry::trace::Span>>(extract_ctx_val);
          op.parent = parent_span->GetContext();
          start_new_trace = true;
        }

        // 不需要启动一个新trace
        if (!start_new_trace) {
          h(filter_data, ctx_ref, msg);
          return;
        }

        // 需要启动一个新trace
        std::string span_name =
            std::string(ctx_ref.GetMetaValue(AIMRT_CHANNEL_CONTEXT_TOPIC_NAME)) + "/" +
            std::string(filter_data.msg_type);
        auto span = tracer->StartSpan(ToNoStdStringView(span_name), op);

        // 将当前span的context打包
        opentelemetry::context::Context output_ot_ctx(trace_api::kSpanKey, span);
        propagator_->Inject(carrier, output_ot_ctx);

        // 添加context中的属性
        auto keys = ctx_ref.GetMetaKeys();
        for (auto& itr : keys) {
          span->SetAttribute(ToNoStdStringView(itr), ToNoStdStringView(ctx_ref.GetMetaValue(itr)));
        }

        h(filter_data, ctx_ref, msg);

        // 序列化包成json
        auto s = filter_data.type_support_ref.SimpleSerialize("json", msg);
        if (!s.empty()) span->SetAttribute("msg_data", s);

        span->End();
      });
}

void OpenTelemetryPlugin::RegisterRpcFilter() {
  auto& rpc_manager = core_ptr_->GetRpcManager();

  rpc_manager.SetPassedContextMetaKeys(
      {std::string(kCtxKeyStartNewTrace),
       std::string(kCtxKeyTraceParent),
       std::string(kCtxKeyTraceState)});

  rpc_manager.RegisterClientFilter(
      "otp_trace",
      [this](const aimrt::runtime::core::rpc::FrameworkFilterData& filter_data,
             aimrt::rpc::ContextRef ctx_ref,
             const void* req,
             void* rsp,
             std::function<void(aimrt::rpc::Status)>&& callback,
             const aimrt::runtime::core::rpc::FrameworkAsyncRpcHandle& h) {
        // 如果context强制设置了start_new_trace，或者上层传递了span，则新启动一个span
        std::string start_new_trace_meta_val(ctx_ref.GetMetaValue(kCtxKeyStartNewTrace));
        bool start_new_trace = (common::util::StrToLower(start_new_trace_meta_val) == "true");

        auto tracer = provider_->GetTracer(options_.node_name);
        ContextCarrier carrier(ctx_ref);

        // 解压传进来的context，得到父span
        trace_api::StartSpanOptions op{
            .kind = trace_api::SpanKind::kClient,
        };

        opentelemetry::context::Context input_ot_ctx;
        auto extract_ctx = propagator_->Extract(carrier, input_ot_ctx);

        auto extract_ctx_val = extract_ctx.GetValue(trace_api::kSpanKey);
        if (!::opentelemetry::nostd::holds_alternative<::opentelemetry::nostd::monostate>(extract_ctx_val)) {
          auto parent_span =
              ::opentelemetry::nostd::get<::opentelemetry::nostd::shared_ptr<::opentelemetry::trace::Span>>(extract_ctx_val);
          op.parent = parent_span->GetContext();
          start_new_trace = true;
        }

        // 不需要启动一个新trace
        if (!start_new_trace) {
          h(filter_data, ctx_ref, req, rsp, std::move(callback));
          return;
        }

        // 需要启动一个新trace
        auto span = tracer->StartSpan(ToNoStdStringView(ctx_ref.GetFunctionName()), op);

        // 将当前span的context打包
        opentelemetry::context::Context output_ot_ctx(trace_api::kSpanKey, span);
        propagator_->Inject(carrier, output_ot_ctx);

        // 添加context中的属性
        auto keys = ctx_ref.GetMetaKeys();
        for (auto& itr : keys) {
          span->SetAttribute(ToNoStdStringView(itr), ToNoStdStringView(ctx_ref.GetMetaValue(itr)));
        }

        h(filter_data, ctx_ref, req, rsp,
          [&filter_data, req, rsp, span{std::move(span)}, callback{std::move(callback)}](aimrt::rpc::Status status) {
            if (status.OK()) {
              span->SetStatus(trace_api::StatusCode::kOk);
            } else {
              span->SetStatus(trace_api::StatusCode::kError, status.ToString());
            }

            // 序列化req/rsp为json
            auto req_json = filter_data.req_type_support_ref.SimpleSerialize("json", req);
            if (!req_json.empty()) span->SetAttribute("req_data", req_json);

            auto rsp_json = filter_data.rsp_type_support_ref.SimpleSerialize("json", rsp);
            if (!rsp_json.empty()) span->SetAttribute("rsp_data", rsp_json);

            span->End();

            callback(status);
          });
      });

  rpc_manager.RegisterServerFilter(
      "otp_trace",
      [this](const aimrt::runtime::core::rpc::FrameworkFilterData& filter_data,
             aimrt::rpc::ContextRef ctx_ref,
             const void* req,
             void* rsp,
             std::function<void(aimrt::rpc::Status)>&& callback,
             const aimrt::runtime::core::rpc::FrameworkAsyncRpcHandle& h) {
        // 如果context强制设置了start_new_trace，或者上层传递了span，则新启动一个span
        std::string start_new_trace_meta_val(ctx_ref.GetMetaValue(kCtxKeyStartNewTrace));
        bool start_new_trace = (common::util::StrToLower(start_new_trace_meta_val) == "true");

        auto tracer = provider_->GetTracer(options_.node_name);
        ContextCarrier carrier(ctx_ref);

        // 解压传进来的context，得到父span
        trace_api::StartSpanOptions op{
            .kind = trace_api::SpanKind::kServer,
        };

        opentelemetry::context::Context input_ot_ctx;
        auto extract_ctx = propagator_->Extract(carrier, input_ot_ctx);

        auto extract_ctx_val = extract_ctx.GetValue(trace_api::kSpanKey);
        if (!::opentelemetry::nostd::holds_alternative<::opentelemetry::nostd::monostate>(extract_ctx_val)) {
          auto parent_span =
              ::opentelemetry::nostd::get<::opentelemetry::nostd::shared_ptr<::opentelemetry::trace::Span>>(extract_ctx_val);
          op.parent = parent_span->GetContext();
          start_new_trace = true;
        }

        // 不需要启动一个新trace
        if (!start_new_trace) {
          h(filter_data, ctx_ref, req, rsp, std::move(callback));
          return;
        }

        // 需要启动一个新trace
        auto span = tracer->StartSpan(ToNoStdStringView(ctx_ref.GetFunctionName()), op);

        // 将当前span的context打包
        opentelemetry::context::Context output_ot_ctx(trace_api::kSpanKey, span);
        propagator_->Inject(carrier, output_ot_ctx);

        // 添加context中的属性
        auto keys = ctx_ref.GetMetaKeys();
        for (auto& itr : keys) {
          span->SetAttribute(ToNoStdStringView(itr), ToNoStdStringView(ctx_ref.GetMetaValue(itr)));
        }

        h(filter_data, ctx_ref, req, rsp,
          [&filter_data, req, rsp, span{std::move(span)}, callback{std::move(callback)}](aimrt::rpc::Status status) {
            if (status.OK()) {
              span->SetStatus(trace_api::StatusCode::kOk);
            } else {
              span->SetStatus(trace_api::StatusCode::kError, status.ToString());
            }

            // 序列化req/rsp为json
            auto req_json = filter_data.req_type_support_ref.SimpleSerialize("json", req);
            if (!req_json.empty()) span->SetAttribute("req_data", req_json);

            auto rsp_json = filter_data.rsp_type_support_ref.SimpleSerialize("json", rsp);
            if (!rsp_json.empty()) span->SetAttribute("rsp_data", rsp_json);

            span->End();

            callback(status);
          });
      });
}

}  // namespace aimrt::plugins::opentelemetry_plugin
