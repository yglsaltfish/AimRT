# mqtt plugin examples

## protobuf rpc co

一个基于 protobuf 协议、协程型接口与 mqtt 后端的 rpc 示例，演示内容包括：
- 如何在配置文件中设置，以使用 mqtt 类型的 rpc 后端；



核心代码：
- [rpc.proto](../../../protocols/example/rpc.proto)
- [normal_rpc_co_client_module.cc](./module/normal_rpc_co_client_module/normal_rpc_co_client_module.cc)
- [normal_rpc_co_server_module.cc](./module/normal_rpc_co_server_module/normal_rpc_co_server_module.cc)
- [service.cc](./module/normal_rpc_co_server_module/service.cc)
- [protobuf_rpc_client_pkg/pkg_main.cc](./pkg/protobuf_rpc_client_pkg/pkg_main.cc)
- [protobuf_rpc_server_pkg/pkg_main.cc](./pkg/protobuf_rpc_server_pkg/pkg_main.cc)


配置文件：
- [examples_plugins_mqtt_plugin_protobuf_rpc_client_cfg.yaml](./install/linux/bin/cfg/examples_plugins_mqtt_plugin_protobuf_rpc_client_cfg.yaml)
- [examples_plugins_mqtt_plugin_protobuf_rpc_server_cfg.yaml](./install/linux/bin/cfg/examples_plugins_mqtt_plugin_protobuf_rpc_server_cfg.yaml)


运行方式（linux）：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 开启 `AIMRT_BUILD_MQTT_PLUGIN` 选项编译 AimRT；
- 编译成功后启动emqx； 
- 在终端运行 build 目录下`start_examples_plugins_mqtt_plugin_protobuf_rpc_server.sh`脚本启动服务端（srv进程）；
- 开启新的终端运行 build 目录下`start_examples_plugins_mqtt_plugin_protobuf_rpc_client.sh`脚本启动客户端（cli进程）；
- 分别在两个终端键入`ctrl-c`停止对应进程；


说明：
- 此示例创建了以下两个模块：
  - `NormalRpcCoClientModule`：会基于 `work_thread_pool` 执行器，以配置的频率，通过协程 Client 接口，向 `ExampleService` 发起 RPC 请求；
  - `NormalRpcCoServerModule`：会注册 `ExampleService` 服务端，通过协程 Server 接口，提供 echo 功能；
- 此示例在 Rpc Client 端和 Server 端分别注册了两个 Filter 用于打印请求日志和计算耗时；
- 此示例将 `NormalRpcCoClientModule` 和 `NormalRpcCoServerModule` 分别集成到 `protobuf_rpc_client_pkg` 和 `protobuf_rpc_server_pkg` 两个 Pkg 中，并在两个配置文件中分别加载对应的 Pkg 到 srv 和 cli 进程中；
- 此示例使用 mqtt 类型的 rpc 后端进行通信，并配置"tcp://127.0.0.1:1883"作为broker,此外还在客户端配置了 `timeout_handle` 执行器作为超时执行器；
- 在配置文件中，新增了 plugin 配置项用来配置 mqtt 插件， 此外还要在 rpc 配置项中引入 mqtt 插件；
 

## ros2 rpc co

一个基于 ros2 srv 协议、协程型接口与 mqtt 后端的 rpc 示例，演示内容包括：
- 如何在配置文件中设置，以使用 mqtt 类型的 rpc 后端；



核心代码：
- [example_ros2/srv/RosTestRpc.srv](../../../protocols/example_ros2/srv/RosTestRpc.srv)
- [normal_rpc_co_client_module.cc](./module/normal_rpc_co_client_module/normal_rpc_co_client_module.cc)
- [normal_rpc_co_server_module.cc](./module/normal_rpc_co_server_module/normal_rpc_co_server_module.cc)
- [service.cc](./module/normal_rpc_co_server_module/service.cc)
- [ros2_rpc_client_pkg/pkg_main.cc](./pkg/ros2_rpc_client_pkg/pkg_main.cc)
- [ros2_rpc_server_pkg/pkg_main.cc](./pkg/ros2_rpc_server_pkg/pkg_main.cc)


配置文件：
- [examples_plugins_mqtt_plugin_ros2_rpc_server_cfg.yaml](./install/linux/bin/cfg/examples_plugins_mqtt_plugin_ros2_rpc_server_cfg.yaml)
- [examples_plugins_mqtt_plugin_ros2_rpc_client_cfg.yaml](./install/linux/bin/cfg/examples_plugins_mqtt_plugin_ros2_rpc_client_cfg.yaml)


运行方式（linux）：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 开启 `AIMRT_BUILD_MQTT_PLUGIN` 选项编译 AimRT；
- 编译成功后启动emqx； 
- 在终端运行 build 目录下`start_examples_plugins_mqtt_plugin_ros2_rpc_server.sh`脚本启动服务端（srv进程）；
- 开启新的终端运行 build 目录下`start_examples_plugins_mqtt_plugin_ros2_rpc_client.sh`脚本启动客户端（cli进程）；
- 分别在两个终端键入`ctrl-c`停止对应进程；


说明：
- 此示例创建了以下两个模块：
  - `NormalRpcCoClientModule`：会基于 `work_thread_pool` 执行器，以配置的频率，通过协程 Client 接口，向 `ExampleService` 发起 RPC 请求；
  - `NormalRpcCoServerModule`：会注册 `ExampleService` 服务端，通过协程 Server 接口，提供 echo 功能；
- 此示例在 Rpc Client 端和 Server 端分别注册了两个 Filter 用于打印请求日志和计算耗时；
- 此示例将 `NormalRpcCoClientModule` 和 `NormalRpcCoServerModule` 分别集成到 `ros2_rpc_client_pkg` 和 `ros2_rpc_server_pkg` 两个 Pkg 中，并在配置文件中加载这两个 Pkg 到一个 AimRT 进程中；
- 此示例使用 mqtt 类型的 rpc 后端进行通信，并配置"tcp://127.0.0.1:1883"作为broker,此外还在客户端配置了 `timeout_handle` 执行器作为超时执行器；
- 在配置文件中，新增了 plugin 配置项用来配置 mqtt 插件， 此外还要在 rpc 配置项中引入 mqtt 插件；




## protobuf channel

一个最基本的、基于 protobuf 协议与 mqtt 后端的 channel 示例，演示内容包括：
- 如何在配置文件中设置，以使用 mqtt 类型的 channel 后端；



核心代码：
- [event.proto](../../../protocols/example/event.proto)
- [normal_publisher_module.cc](./module/normal_publisher_module/normal_publisher_module.cc)
- [normal_subscriber_module.cc](./module/normal_subscriber_module/normal_subscriber_module.cc)
- [protobuf_channel_pub_pkg/pkg_main.cc](./pkg/protobuf_channel_pub_pkg/pkg_main.cc)
- [protobuf_channel_sub_pkg/pkg_main.cc](./pkg/protobuf_channel_sub_pkg/pkg_main.cc)

配置文件：
- [examples_plugins_mqtt_plugin_protobuf_channel_pub_cfg.yaml](./install/linux/bin/cfg/examples_plugins_mqtt_plugin_protobuf_channel_pub_cfg.yaml)
- [examples_plugins_mqtt_plugin_protobuf_channel_sub_cfg.yaml](./install/linux/bin/cfg/examples_plugins_mqtt_plugin_protobuf_channel_sub_cfg.yaml)

运行方式（linux）：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 开启 `AIMRT_BUILD_MQTT_PLUGIN` 选项编译 mqtt 插件；
- 编译成功后启动emqx； 
- 在终端运行 build 目录下`start_examples_plugins_mqtt_plugin_protobuf_channel_sub.sh`脚本启动订阅端（sub进程）；
- 再开启一个新的终端窗口运行`start_examples_plugins_mqtt_plugin_protobuf_channel_pub.sh`脚本启动发布端（pub进程）；
- 分别在开启的三个终端键入`ctrl-c`停止对应进程；


说明：
- 此示例创建了以下两个模块：
  - `NormalPublisherModule`：会基于 `work_thread_pool` 执行器，以配置的频率、向配置的 topic 中发布 `ExampleEventMsg` 类型的消息；
  - `NormalSubscriberModule`：会订阅配置的 topic 下的 `ExampleEventMsg` 类型的消息；
- 此示例将 `NormalPublisherModule` 和 `NormalSubscriberModule` 分别集成到 `protobuf_channel_pub_pkg` 和 `protobuf_channel_sub_pkg` 两个 Pkg 中，并在两个配置文件中分别加载对应的 Pkg 到 pub 和 sub 进程中；
- 此示例使用 mqtt 类型的 channel 后端进行通信,并配置"tcp://127.0.0.1:1883"作为broker；
- 在配置文件中，新增了 plugin 配置项用来配置 mqtt 插件， 此外还要在 channel 配置项中引入 mqtt 插件

## ros2 channel

一个最基本的、基于 ros2 msg 协议与 ros2 后端的 channel 示例，演示内容包括：

- 如何在配置文件中设置，以使用 mqtt 类型的 channel 后端；


核心代码：
- [example_ros2/msg/RosTestMsg.msg](../../../protocols/example_ros2/msg/RosTestMsg.msg)
- [normal_publisher_module.cc](./module/normal_publisher_module/normal_publisher_module.cc)
- [normal_subscriber_module.cc](./module/normal_subscriber_module/normal_subscriber_module.cc)
- [ros2_channel_pub_pkg/pkg_main.cc](./pkg/ros2_channel_pub_pkg/pkg_main.cc)
- [ros2_channel_sub_pkg/pkg_main.cc](./pkg/ros2_channel_sub_pkg/pkg_main.cc)


配置文件：
- [examples_plugins_mqtt_plugin_ros2_channel_pub_cfg.yaml](./install/linux/bin/cfg/examples_plugins_mqtt_plugin_ros2_channel_pub_cfg.yaml)
- [examples_plugins_mqtt_plugin_ros2_channel_sub_cfg.yaml](./install/linux/bin/cfg/examples_plugins_mqtt_plugin_ros2_channel_sub_cfg.yaml)

运行方式（linux）：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 开启 `AIMRT_BUILD_MQTT_PLUGIN` 选项编译 mqtt 插件；
- 编译成功后启动emqx； 
- 在终端运行 build 目录下`start_examples_plugins_mqtt_plugin_ros2_channel_sub.sh`脚本启动订阅端（sub进程）；
- 再开启一个新的终端窗口运行`start_examples_plugins_mqtt_plugin_ros2_channel_pub.sh`脚本启动发布端（pub进程）；
- 分别在开启的三个终端键入`ctrl-c`停止对应进程；


说明：
- 此示例创建了以下两个模块：
  - `NormalPublisherModule`：会基于 `work_thread_pool` 执行器，以配置的频率、向配置的 topic 中发布 `RosTestMsg` 类型的消息；
  - `NormalSubscriberModule`：会订阅配置的 topic 下的 `RosTestMsg` 类型的消息；
- 此示例将 `NormalPublisherModule` 和 `NormalSubscriberModule` 分别集成到 `ros2_channel_pub_pkg` 和 `ros2_channel_sub_pkg` 两个 Pkg 中，并在配置文件中加载这两个 Pkg 到一个 AimRT 进程中；
- 此示例使用 mqtt 类型的 channel 后端进行通信,并配置"tcp://127.0.0.1:1883"作为broker；
- 在配置文件中，新增了 plugin 配置项用来配置 mqtt 插件， 此外还要在 channel 配置项中引入 mqtt 插件