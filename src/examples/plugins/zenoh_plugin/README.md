# zenoh plugin examples

## protobuf channel

一个最基本的、基于 protobuf 协议与 zenoh 后端的 channel 示例，演示内容包括：
- 如何在配置文件中设置，以使用 zenoh 类型的 channel 后端；



核心代码：
- [event.proto](../../../protocols/example/event.proto)
- [normal_publisher_module.cc](./module/normal_publisher_module/normal_publisher_module.cc)
- [normal_subscriber_module.cc](./module/normal_subscriber_module/normal_subscriber_module.cc)
- [protobuf_channel_pub_pkg/pkg_main.cc](./pkg/protobuf_channel_pub_pkg/pkg_main.cc)
- [protobuf_channel_sub_pkg/pkg_main.cc](./pkg/protobuf_channel_sub_pkg/pkg_main.cc)

配置文件：
- [examples_plugins_zenoh_plugin_protobuf_channel_pub_cfg.yaml](./install/linux/bin/cfg/examples_plugins_zenoh_plugin_protobuf_channel_pub_cfg.yaml)
- [examples_plugins_zenoh_plugin_protobuf_channel_sub_cfg.yaml](./install/linux/bin/cfg/examples_plugins_zenoh_plugin_protobuf_channel_sub_cfg.yaml)

运行方式（linux）：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 开启 `AIMRT_BUILD_ZENOH_PLUGIN` 选项编译 zenoh 插件（编译时需要提前准备好rust编译环境, 否则会自动关闭该选项）；
- 编译成功后，先运行 build 目录下`start_examples_plugins_zenoh_plugin_protobuf_channel_sub.sh`脚本启动订阅端（sub进程）；
- 再开启一个新的终端窗口运行`start_examples_plugins_zenoh_plugin_protobuf_channel_pub.sh`脚本启动发布端（pub进程）；
- 分别在两个终端键入`ctrl-c`停止对应进程；


说明：
- 此示例创建了以下两个模块：
  - `NormalPublisherModule`：会基于 `work_thread_pool` 执行器，以配置的频率、向配置的 topic 中发布 `ExampleEventMsg` 类型的消息；
  - `NormalSubscriberModule`：会订阅配置的 topic 下的 `ExampleEventMsg` 类型的消息；
- 此示例将 `NormalPublisherModule` 和 `NormalSubscriberModule` 分别集成到 `protobuf_channel_pub_pkg` 和 `protobuf_channel_sub_pkg` 两个 Pkg 中，并在两个配置文件中分别加载对应的 Pkg 到 pub 和 sub 进程中；
- 此示例使用 zenoh 类型的 channel 后端进行通信；
- 在配置文件中，新增了 plugin配置项用来配置 zenoh 插件， 此外还要在 channel配置项中引入 zenoh 插件
