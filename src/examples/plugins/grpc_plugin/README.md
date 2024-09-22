# grpc plugin examples

## protobuf rpc

一个基于 protobuf 协议、协程型接口与 grpc 后端的 rpc 示例，演示内容包括：
- 如何在配置文件中设置，以使用 grpc 类型的 rpc 后端；



核心代码：
- [rpc.proto](../../../protocols/example/rpc.proto)
- [normal_rpc_co_client_module.cc](./module/normal_rpc_co_client_module/normal_rpc_co_client_module.cc)
- [normal_rpc_co_server_module.cc](./module/normal_rpc_co_server_module/normal_rpc_co_server_module.cc)
- [service.cc](./module/normal_rpc_co_server_module/service.cc)
- [protobuf_rpc_client_pkg/pkg_main.cc](./pkg/protobuf_rpc_client_pkg/pkg_main.cc)
- [protobuf_rpc_server_pkg/pkg_main.cc](./pkg/protobuf_rpc_server_pkg/pkg_main.cc)


配置文件：
- [examples_plugins_grpc_plugin_protobuf_rpc_client_cfg.yaml](./install/linux/bin/cfg/examples_plugins_grpc_plugin_protobuf_rpc_client_cfg.yaml)
- [examples_plugins_grpc_plugin_protobuf_rpc_server_cfg.yaml](./install/linux/bin/cfg/examples_plugins_grpc_plugin_protobuf_rpc_server_cfg.yaml)


运行方式（linux）：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 开启 `AIMRT_BUILD_GRPC_PLUGIN` 选项编译 AimRT；
- 编译成功后在终端运行 build 目录下`start_examples_plugins_grpc_plugin_protobuf_rpc_server.sh`脚本启动服务端（srv进程）；
- 开启新的终端运行 build 目录下`start_examples_plugins_grpc_plugin_protobuf_rpc_client.sh`脚本启动客户端（cli进程）；
- 分别在两个终端键入`ctrl-c`停止对应进程；


说明：
- 此示例创建了以下两个模块：
  - `NormalRpcCoClientModule`：会基于 `work_thread_pool` 执行器，以配置的频率，通过协程 Client 接口，向 `ExampleService` 发起 RPC 请求；
  - `NormalRpcCoServerModule`：会注册 `ExampleService` 服务端，通过协程 Server 接口，提供 echo 功能；
- 此示例在 Rpc Client 端和 Server 端分别注册了两个 Filter 用于打印请求日志和计算耗时；
- 此示例将 `NormalRpcCoClientModule` 和 `NormalRpcCoServerModule` 分别集成到 `protobuf_rpc_client_pkg` 和 `protobuf_rpc_server_pkg` 两个 Pkg 中，并在配置文件中加载这两个 Pkg 到一个 AimRT 进程中；
- 此示例使用 grpc 类型的 rpc 后端进行通信，并配置"127.0.0.1:50051"作为客户端地址， "127.0.0.1:50050"作为服务端地址；
- 在配置文件中，新增了 plugin 配置项用来配置 grpc 插件， 此外还要在 rpc 配置项中引入 grpc 插件；




