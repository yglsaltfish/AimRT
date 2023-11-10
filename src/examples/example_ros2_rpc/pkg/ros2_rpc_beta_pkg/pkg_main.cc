#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "ros2_rpc_server_module/ros2_rpc_server_module.h"

static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"Ros2RpcServerModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::example_ros2_rpc::ros2_rpc_server_module::
               Ros2RpcServerModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
