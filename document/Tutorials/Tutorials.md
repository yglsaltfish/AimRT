
[TOC]


# 基本概念


## Modern CMake
&emsp;&emsp;`AimRT`框架使用Modern CMake进行构建，每个叶子文件夹是一个CMake Target。库之间相互引用时能自动引用下层级的所有依赖。

&emsp;&emsp;关于Modern CMake详细使用方式此处不做赘述，您可以参考一些其他教程：
- [More Modern CMake](https://hsf-training.github.io/hsf-training-cmake-webpage/aio/index.html)
- [Effective Modern CMake](https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1)
- [An Introduction to Modern CMake](https://cliutils.gitlab.io/modern-cmake/)
- [cmake官方文档add_library页面](https://cmake.org/cmake/help/latest/command/add_library.html)
- [cmake学习例子集合](https://github.com/ttroy50/cmake-examples)

## `Module`：模块
&emsp;&emsp;`Module`代表一个可被框架在运行时加载的类，通过实现几个简单的模块接口来创建。一个`Module`通常对应一个硬件抽象，或者是一个独立算法、一项业务功能。`Module`可以使用框架提供的各项运行时功能，例如配置、日志、执行器等。

&emsp;&emsp;`Module`之间通信主要有两种方式：
- Channel
- RPC

## `Channel`：数据通道
&emsp;&emsp;`Channel`通过`Topic`标识单个数据通道，由发布者`Publisher`和订阅者`Subscriber`组成，`Module`可以向任意数量的`Topic`发布数据，同时可以订阅任意数量的`Topic`。类似的概念如ROS中的`Topic`、Kafka/RabbitMQ等消息队列。

&emsp;&emsp;在`AimRT`中，`Channel`更侧重于一种上层通信逻辑接口，其底层使用的通信方式可以根据配置而定。

## `Rpc`：远程过程调用
&emsp;&emsp;RPC基于请求-回复模型，由客户端`Client`和服务端`Server`组成，`Module`可以创建客户端句柄，发起特定的RPC请求，由其指定的、或由框架根据一定规则指定的服务端来接收请求并回复。`Module`也可以创建服务端句柄，提供特定的RPC服务，接收处理系统路由过来的请求并回复。类似的概念如ROS中的`Services`、GRPC/Thrift等RPC框架。

&emsp;&emsp;在`AimRT`中，`Rpc`更侧重于一种上层通信逻辑接口，其底层使用的通信方式可以根据配置而定。

## `Protocol`：协议
&emsp;&emsp;`Protocol`代表`Module`之间通信的数据格式，用来描述数据的字段信息。通常由一种IDL(Interface description language)描述，然后由某种工具转换为各个语言的代码。

&emsp;&emsp;`AimRT`目前支持两种IDL：
- protobuf
- ros2 msg/srv

## `Executor`：执行器
&emsp;&emsp;`Executor`是指一个可以运行任务的抽象概念，一个执行器可以是一个Fiber、Thread或者Thread Pool，我们平常写的代码也是默认的直接指定了一个执行器：Main线程。`AimRT`默认提供了Thread和Thread Pool类型的`Executor`，同时支持自定义`Executor`类型。

## `Pkg`：模块包
&emsp;&emsp;`Pkg`代表一个包含了单个或多个`Module`的动态库，可以被框架运行时加载。`Pkg`通过实现几个简单的模块描述接口来创建。一个`Pkg`中可以有多个`Module`，相比于`Pkg`，`Module`的概念更侧重于代码层面，而`Pkg`则是一个部署层面的概念，其中不包含业务逻辑代码。一般来说，在可以互相兼容的情况下，推荐将多个`Module`编译在一个`Pkg`中，这种情况下使用RPC、Channel等功能时性能会有优化。

&emsp;&emsp;`Pkg`中的符号都是默认隐藏的，只暴露有限的纯C接口，不同`Pkg`之间不会有符号上的相互干扰。一般情况下，不同`Pkg`可以使用不同版本的编译器独立编译，不同`Pkg`里的`Module`也可以使用相互冲突的第三方依赖进行编译，最终编译出的动态库可以二进制发布，最终这些`Pkg`可以被加载到同一个框架运行时进程中。

## `Node`：节点
&emsp;&emsp;`Node`代表一个可以部署启动的进程，在其中运行了一个`AimRT`框架的Runtime实例。一个`Node`通过配置文件来加载一个或多个`Pkg`，并根据配置文件加载这些`Pkg`中的一个或多个`Module`。`Node`在启动时还可以通过配置文件来设置日志、插件、执行器等功能。多个`Node`之间可以通过底层通信组件提供的通信方式（如GRpc、ROS2、共享内存、tcp/udp等）进行组网通信。

## `Plugin`：插件
&emsp;&emsp;`Plugin`是指一个可以向`AimRT`框架注册各种自定义功能的动态库，可以被框架运行时加载。`AimRT`框架暴露了大量插接点和查询接口，如日志后端注册接口、Channel后端注册接口、Rpc注册表查询接口等，用户可以在`Plugin`中实现一些自定义功能增强框架的服务能力。

&emsp;&emsp;使用者可以直接使用一些`AimRT`官方提供的插件，也可以从第三方开发者处寻求一些插件以满足特定需求。`AimRT`官方提供的插件有以下几种：
- ros2_plugin：提供ros2的channel与rpc后端，使`AimRT`能够兼容Ros2。
- lcm_plugin：提供基于lcm的channel与rpc后端。
- net_plugin：提供http、tcp、udp的channel与rpc后端。
- sm_plugin：提供基于共享内存的channel与rpc后端。


# 基本使用


## 使用Modern CMake构建项目

&emsp;&emsp;Modern CMake是目前主流C++项目的常用构建方式，其使用面向对象的构建思路，引入了Target、属性等概念，将包括依赖在内的各个参数信息全部封装起来，大大简化了依赖管理，使构建大型系统更有条理、更加轻松。


## 使用脚手架工具创建helloworld项目

参考以下配置文件：
```yaml


```

在源码库 `src/tools/aimrt_cli/aimrt_cli`下找到示例配置文件 `configuration_example.yaml`，在命令行中运行
```
aimrt_cli gen -p path/to/configuration_example.yaml -o path/to/your_desire
```
即可生成一套基础示例工程。运行生成项目的`build.sh`进行编译。

具体环境配置以及aimrt_cli工具介绍详见[aimrt_cli工具详解](./aimrt_cli_introduction.md)


## 使用配置功能



## 使用日志功能



## 使用执行器



## 使用channel通信


## 使用rpc通信


## 使用插件


## 参考示例




# 进阶使用

## aimrt_cli工具详细介绍
aimrt_cli工具详细介绍及使用指引详见: [aimrt_cli工具详解](./aimrt_cli_introduction.md)

## 协程

## 执行器实时性支持


## 插件开发


## 官方插件详细介绍

1. [LCM插件](./LcmPlugin.md)

2. [共享内存插件](./SmPlugin.md)

## 通用协议


## 接口层


## 测试


## 时间快进/慢放



# 示例


