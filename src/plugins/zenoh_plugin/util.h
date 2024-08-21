// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include <string>

namespace aimrt::plugins::zenoh_plugin {

// note:keyexpr cantnot begin with "/", but ulr can.
inline std::string Url2Keyexpr(const std::string& url) {
  return url.substr(1);
}

inline std::string Keyexpr2Url(const std::string& keyexpr) {
  return "/" + keyexpr;
}

}  // namespace aimrt::plugins::zenoh_plugin