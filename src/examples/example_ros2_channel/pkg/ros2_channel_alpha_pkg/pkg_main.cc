#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "ros2_publisher_module/ros2_publisher_module.h"

static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"Ros2PublisherModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::example_ros2_channel::
               ros2_publisher_module::Ros2PublisherModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
