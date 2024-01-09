
#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "benchmark_publisher_module/benchmark_publisher_module.h"

static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"BenchmarkPublisherModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::example_normal_channel::benchmark_publisher_module::BenchmarkPublisherModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
