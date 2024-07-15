
# AimRT中的CMake


AimRT使用原生标准的Modern CMake进行构建，并基于CMake Fetchcontent获取依赖。

## Modern CMake
Modern CMake是目前主流C++项目的常用构建方式，使用面向对象的构建思路，引入了Target、属性等概念，可以将包括依赖在内的各个参数信息全部封装起来，大大简化了依赖管理，使构建大型系统更有条理、更加轻松。`AimRT`框架使用Modern CMake进行构建，每个叶子文件夹是一个CMake Target。库之间相互引用时能自动处理下层级的所有依赖。

关于Modern CMake详细使用方式此处不做赘述，您可以参考一些其他教程：
- [CMake official website](https://cmake.org/cmake/help/latest/command/add_library.html)
- [More Modern CMake](https://hsf-training.github.io/hsf-training-cmake-webpage/aio/index.html)
- [Effective Modern CMake](https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1)
- [An Introduction to Modern CMake](https://cliutils.gitlab.io/modern-cmake/)

## AimRT的第三方依赖管理策略
当您准备从源码引用/构建AimRT时，您需要了解AimRT第三方依赖管理策略：
- AimRT使用CMake FetchContent拉取依赖，关于CMake FetchContent的详细使用方式，请参考[CMake官方文档](https://cmake.org/cmake/help/latest/module/FetchContent.html)。
- AimRT默认从各个第三方依赖的官方下载地址进行下载，如果你想通过自定义的下载地址下载这些依赖，可以参考`cmake/GetXXX.cmake`中的代码，在构建时传入`-DXXX_DOWNLOAD_URL`参数，将下载url修改为您自定义的地址。
- 如果您的构建环境无法连接外部网络，你也可以离线的下载这些依赖，然后参考`cmake/GetXXX.cmake`中的代码，在构建时传入`-DXXX_LOCAL_SOURCE`参数，将依赖寻找地址转为您指定的本地地址。
- 如果以上方式还不满足您对依赖管理的自定义需求，您也可以直接自定义`cmake/GetXXX.cmake`中的代码，只要引入满足AimRT构建所需的CMake Target即可。
- 请注意，AimRT仅验证了默认参数中配置的各个第三方依赖的版本，如果您需要升级或降级这些第三方依赖的版本，请自行保证兼容性和稳定性。

## AimRT的CMake选项
AimRT框架由其interface层、runtime主体，加上多个插件、工具共同组成，在构建时可以通过配置CMake选项，选择其中一部分或全部进行构建。详细的CMake选项列表如下：

|  CMake Option名称                     | 类型  | 默认值 | 作用 |
|  ----                                 | ----  | ----  | ----  |
|  AIMRT_BUILD_TESTS                    | BOOL  | OFF   | 是否编译测试  |
|  AIMRT_BUILD_EXAMPLES                 | BOOL  | OFF   | 是否编译示例  |
|  AIMRT_BUILD_DOCUMENT                 | BOOL  | OFF   | 是否构建文档  |
|  AIMRT_BUILD_RUNTIME                  | BOOL  | ON    | 是否编译运行时  |
|  AIMRT_BUILD_CLI_TOOLS                | BOOL  | OFF   | 是否编译cli工具  |
|  AIMRT_BUILD_PYTHON_RUNTIME           | BOOL  | OFF   | 是否编译Python运行时  |
|  AIMRT_USE_FMT_LIB                    | BOOL  | ON    | 是否使用Fmt库  |
|  AIMRT_BUILD_WITH_PROTOBUF            | BOOL  | ON    | 是否使用Protobuf库  |
|  AIMRT_USE_LOCAL_PROTOC_COMPILER      | BOOL  | OFF   | 是否使用本地的protoc工具  |
|  AIMRT_USE_PROTOC_PYTHON_PLUGIN       | BOOL  | OFF   | 是否使用Python版本protoc插件  |
|  AIMRT_BUILD_WITH_ROS2                | BOOL  | OFF   | 是否使用ROS2 Humble  |
|  AIMRT_BUILD_NET_PLUGIN               | BOOL  | OFF   | 是否编译Net插件  |
|  AIMRT_BUILD_ROS2_PLUGIN              | BOOL  | OFF   | 是否编译ROS2 Humble插件  |
|  AIMRT_BUILD_MQTT_PLUGIN              | BOOL  | OFF   | 是否编译Mqtt插件  |
|  AIMRT_BUILD_RECORD_PLAYBACK_PLUGIN   | BOOL  | OFF   | 是否编译录播插件  |
|  AIMRT_BUILD_TIME_MANIPULATOR_PLUGIN  | BOOL  | OFF   | 是否编译time manipulator插件  |
|  AIMRT_BUILD_PARAMETER_PLUGIN         | BOOL  | OFF   | 是否编译parameter插件  |
|  AIMRT_BUILD_LOG_CONTROL_PLUGIN       | BOOL  | OFF   | 是否编译日志控制插件  |
|  AIMRT_INSTALL                        | BOOL  | ON    | 是否需要install aimrt |


## AimRT中的CMake Target
AimRT中所有的可引用的非协议类型CMake Target如下：

|  CMake Target名称                                 | 作用  | 需要开启的宏 |
|  ----                                             | ----  | ----  |
| aimrt::common::util                               | 一些独立基础工具，如string、log等 |  |
| aimrt::common::ros2_util                          | 独立的ros2相关的基础工具 | AIMRT_BUILD_WITH_ROS2  |
| aimrt::interface::aimrt_module_c_interface        | 模块开发接口-C版本 |   |
| aimrt::interface::aimrt_module_cpp_interface      | 模块开发接口-CPP版本 |   |
| aimrt::interface::aimrt_module_protobuf_interface | 模块开发protobuf相关接口，基于CPP接口 | AIMRT_BUILD_WITH_PROTOBUF  |
| aimrt::interface::aimrt_module_ros2_interface     | 模块开发ros2相关接口，基于CPP接口 | AIMRT_BUILD_WITH_ROS2  |
| aimrt::interface::aimrt_pkg_c_interface           | Pkg开发接口 |   |
| aimrt::interface::aimrt_core_plugin_interface     | 插件开发接口 | AIMRT_BUILD_RUNTIME  |
| aimrt::runtime::core                              | 运行时核心库 | AIMRT_BUILD_RUNTIME  |

