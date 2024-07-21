#pragma once

#include "aimrt_module_cpp_interface/rpc/rpc_context.h"
#include "opentelemetry_plugin/util.h"

#include "opentelemetry/context/propagation/text_map_propagator.h"

namespace aimrt::plugins::opentelemetry_plugin {

class RpcContextCarrier : public opentelemetry::context::propagation::TextMapCarrier {
 public:
  explicit RpcContextCarrier(aimrt::rpc::ContextRef ctx_ref)
      : ctx_ref_(ctx_ref) {}
  RpcContextCarrier() = default;

  virtual opentelemetry::nostd::string_view Get(
      opentelemetry::nostd::string_view key) const noexcept override {
    std::string real_key = std::string(kCtxKeyPrefix) + std::string(key);
    return ToNoStdStringView(ctx_ref_.GetMetaValue(real_key));
  }

  virtual void Set(opentelemetry::nostd::string_view key,
                   opentelemetry::nostd::string_view value) noexcept override {
    std::string real_key = std::string(kCtxKeyPrefix) + std::string(key);
    ctx_ref_.SetMetaValue(real_key, ToStdStringView(value));
  }

  aimrt::rpc::ContextRef ctx_ref_;
};
}  // namespace aimrt::plugins::opentelemetry_plugin