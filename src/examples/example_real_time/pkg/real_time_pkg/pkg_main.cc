#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "real_time_module/real_time_module.h"

static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"RealTimeModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::example_real_time::real_time_module::RealTimeModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
