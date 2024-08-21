// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#include <cstring>

#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "normal_rpc_co_server_module/normal_rpc_co_server_module.h"

static std::tuple<std::string_view, std::function<aimrt::ModuleBase*()>> aimrt_module_register_array[]{
    {"NormalRpcCoServerModule", []() -> aimrt::ModuleBase* {
       return new aimrt::examples::cpp::protobuf_rpc::normal_rpc_co_server_module::NormalRpcCoServerModule();
     }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
