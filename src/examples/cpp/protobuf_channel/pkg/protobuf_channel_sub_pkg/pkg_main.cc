#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "benchmark_publisher_module/benchmark_publisher_module.h"
#include "benchmark_subscriber_module/benchmark_subscriber_module.h"
#include "normal_publisher_module/normal_publisher_module.h"
#include "normal_subscriber_module/normal_subscriber_module.h"

static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"NormalSubscriberModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::cpp::protobuf_channel::normal_subscriber_module::NormalSubscriberModule();
         }},
        {"BenchmarkSubscriberModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::cpp::protobuf_channel::benchmark_subscriber_module::BenchmarkSubscriberModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
