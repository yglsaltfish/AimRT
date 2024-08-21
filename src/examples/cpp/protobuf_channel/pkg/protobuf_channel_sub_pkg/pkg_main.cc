// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "benchmark_publisher_module/benchmark_publisher_module.h"
#include "benchmark_subscriber_module/benchmark_subscriber_module.h"
#include "normal_publisher_module/normal_publisher_module.h"
#include "normal_subscriber_module/normal_subscriber_module.h"

using namespace aimrt::examples::cpp::protobuf_channel;

static std::tuple<std::string_view, std::function<aimrt::ModuleBase*()>> aimrt_module_register_array[]{
    {"NormalSubscriberModule", []() -> aimrt::ModuleBase* {
       return new normal_subscriber_module::NormalSubscriberModule();
     }},
    {"BenchmarkSubscriberModule", []() -> aimrt::ModuleBase* {
       return new benchmark_subscriber_module::BenchmarkSubscriberModule();
     }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
