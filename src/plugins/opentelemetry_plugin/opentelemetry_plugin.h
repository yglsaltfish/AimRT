#pragma once

#include <memory>
#include <vector>

#include "aimrt_core_plugin_interface/aimrt_core_plugin_base.h"

#include <opentelemetry/trace/propagation/http_trace_context.h>
#include "opentelemetry/exporters/otlp/otlp_http_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_options.h"
#include "opentelemetry/sdk/trace/batch_span_processor_factory.h"
#include "opentelemetry/sdk/trace/batch_span_processor_options.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/recordable.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/sdk/version/version.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/scope.h"
#include "opentelemetry/trace/span.h"
#include "opentelemetry/trace/span_context.h"
#include "opentelemetry/trace/tracer.h"
#include "opentelemetry/trace/tracer_provider.h"

namespace aimrt::plugins::opentelemetry_plugin {

class OpenTelemetryPlugin : public AimRTCorePluginBase {
 public:
  struct Options {
    std::string node_name;
    std::string trace_otlp_http_exporter_url;

    struct Attribute {
      std::string key;
      std::string val;
    };
    std::vector<Attribute> attributes;
  };

 public:
  OpenTelemetryPlugin() = default;
  ~OpenTelemetryPlugin() override = default;

  std::string_view Name() const noexcept override { return "opentelemetry_plugin"; }

  bool Initialize(runtime::core::AimRTCore* core_ptr) noexcept override;
  void Shutdown() noexcept override;

 private:
  void SetPluginLogger();
  void RegisterChannelFilter();
  void RegisterRpcFilter();

 private:
  runtime::core::AimRTCore* core_ptr_ = nullptr;

  Options options_;

  bool init_flag_ = false;

  std::shared_ptr<opentelemetry::trace::TracerProvider> provider_;
  std::shared_ptr<opentelemetry::context::propagation::TextMapPropagator> propagator_;
};

}  // namespace aimrt::plugins::opentelemetry_plugin
