// Copyright (c) 2023, AgiBot Inc.
// All rights reserved

#pragma once
#include <string>
namespace aimrt::plugins::zenoh_plugin {
std::string Url2Keyexpr(const std::string& url);
std::string Keyexpr2Url(const std::string& keyexpr);

}  // namespace aimrt::plugins::zenoh_plugin