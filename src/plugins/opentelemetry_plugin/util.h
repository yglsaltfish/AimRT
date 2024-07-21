#pragma once

#include <string_view>

#include "opentelemetry/nostd/string_view.h"

namespace aimrt::plugins::opentelemetry_plugin {

static constexpr std::string_view kCtxKeyPrefix = "aimrt_otp-";

inline opentelemetry::nostd::string_view ToNoStdStringView(std::string_view s) {
  return opentelemetry::nostd::string_view(s.data(), s.size());
}

inline std::string_view ToStdStringView(opentelemetry::nostd::string_view s) {
  return std::string_view(s.data(), s.size());
}

}  // namespace aimrt::plugins::opentelemetry_plugin