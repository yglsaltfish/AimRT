#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "normal_publisher_module/normal_publisher_module.h"
#include "normal_subscriber_module/normal_subscriber_module.h"

static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"NormalPublisherModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::cpp::ros2_channel::normal_publisher_module::NormalPublisherModule();
         }},
        {"NormalSubscriberModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::cpp::ros2_channel::normal_subscriber_module::NormalSubscriberModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
