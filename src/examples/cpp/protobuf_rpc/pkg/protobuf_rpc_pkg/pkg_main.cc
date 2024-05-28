#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "normal_rpc_async_client_module/normal_rpc_async_client_module.h"
#include "normal_rpc_async_server_module/normal_rpc_async_server_module.h"
#include "normal_rpc_client_module/normal_rpc_client_module.h"
#include "normal_rpc_server_module/normal_rpc_server_module.h"
#include "normal_rpc_sync_client_module/normal_rpc_sync_client_module.h"
#include "normal_rpc_sync_server_module/normal_rpc_sync_server_module.h"

using namespace aimrt::examples::cpp::protobuf_rpc;

static std::tuple<std::string_view, std::function<aimrt::ModuleBase*()>> aimrt_module_register_array[]{
    {"NormalRpcClientModule", []() -> aimrt::ModuleBase* {
       return new normal_rpc_client_module::NormalRpcClientModule();
     }},
    {"NormalRpcServerModule", []() -> aimrt::ModuleBase* {
       return new normal_rpc_server_module::NormalRpcServerModule();
     }},
    {"NormalRpcAsyncClientModule", []() -> aimrt::ModuleBase* {
       return new normal_rpc_async_client_module::NormalRpcAsyncClientModule();
     }},
    {"NormalRpcAsyncServerModule", []() -> aimrt::ModuleBase* {
       return new normal_rpc_async_server_module::NormalRpcAsyncServerModule();
     }},
    {"NormalRpcSyncClientModule", []() -> aimrt::ModuleBase* {
       return new normal_rpc_sync_client_module::NormalRpcSyncClientModule();
     }},
    {"NormalRpcSyncServerModule", []() -> aimrt::ModuleBase* {
       return new normal_rpc_sync_server_module::NormalRpcSyncServerModule();
     }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
