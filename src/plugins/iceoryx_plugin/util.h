// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once
#include <iomanip>
#include "iceoryx_plugin/global.h"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

#if defined(_WIN32)
  #include <windows.h>
#else
  #include <unistd.h>
#endif

namespace aimrt::plugins::iceoryx_plugin {

using IdString_t = iox::capro::IdString_t;

constexpr unsigned int FIXED_LEN = 20;        // FIXED_LEN represents the length of the pkg_size's string， which is enough to the max value of uint64_t
constexpr uint64_t IOX_SHM_INIT_SIZE = 1024;  // default vaule of shm_init_size for iceoryx

const iox::capro::ServiceDescription Url2ServiceDescription(std::string& url);

std::string GetPid();

std::string IntToFixedLengthString(int number, int length);

}  // namespace aimrt::plugins::iceoryx_plugin