#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "normal_rpc_server_module/normal_rpc_server_module.h"

static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"NormalRpcServerModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::example_normal_rpc::
               normal_rpc_server_module::NormalRpcServerModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
