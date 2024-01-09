#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "normal_subscriber_module/normal_subscriber_module.h"

static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"NormalSubscriberModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::example_normal_channel::normal_subscriber_module::NormalSubscriberModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
