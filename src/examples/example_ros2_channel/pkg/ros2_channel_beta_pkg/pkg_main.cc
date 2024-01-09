#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "ros2_subscriber_module/ros2_subscriber_module.h"

static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"Ros2SubscriberModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::example_ros2_channel::ros2_subscriber_module::Ros2SubscriberModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
