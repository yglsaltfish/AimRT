
#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "benchmark_subscriber_module/benchmark_subscriber_module.h"

static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"BenchmarkSubscriberModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::example_normal_channel::
               benchmark_subscriber_module::BenchmarkSubscriberModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
