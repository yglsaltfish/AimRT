#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "benchmark_publisher_module/benchmark_publisher_module.h"
#include "benchmark_subscriber_module/benchmark_subscriber_module.h"
#include "normal_publisher_module/normal_publisher_module.h"
#include "normal_subscriber_module/normal_subscriber_module.h"

static std::tuple<std::string_view, std::function<aimrt::ModuleBase*()>>
    aimrt_module_register_array[]{
        {"NormalPublisherModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::cpp::protobuf_channel::normal_publisher_module::NormalPublisherModule();
         }},
        {"BenchmarkPublisherModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::cpp::protobuf_channel::benchmark_publisher_module::BenchmarkPublisherModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
