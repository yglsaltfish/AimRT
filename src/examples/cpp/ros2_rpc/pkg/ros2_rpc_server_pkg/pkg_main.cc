#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "normal_rpc_server_module/normal_rpc_server_module.h"

static std::tuple<std::string_view, std::function<aimrt::ModuleBase*()>>
    aimrt_module_register_array[]{
        {"NormalRpcServerModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::cpp::ros2_rpc::normal_rpc_server_module::NormalRpcServerModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
