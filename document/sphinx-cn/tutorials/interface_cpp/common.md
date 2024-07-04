# Cpp开发接口概述


## 接口层

AimRT为业务逻辑开发提供了一套CPP接口层，CMake Target名称为 **aimrt::interface::aimrt_module_cpp_interface**，在开发`Module`时只需要链接这个接口层库，可以与AimRT的实现细节相隔离。此接口层库的依赖只有两个：
- [fmt](https://github.com/fmtlib/fmt)：用于日志。如果使用C++20的format，则可以去掉这个依赖。
- [libunifex](https://github.com/facebookexperimental/libunifex)：用于将异步逻辑封装为协程。可以选用。


## 接口中的引用类型

CPP接口层中，大部分句柄都是一种引用类型，具有以下特点：
- 类型命名一般以`Ref`结尾。
- 这种引用类型一般比较轻量，拷贝传递不会有较大开销。
- 这种引用类型一般都提供了一个`operator bool()`的重载来判断引用是否有效。
- 调用这种引用类型中提供的接口时，如果引用为空，会抛出一个异常。

## AimRT生命周期以及接口调用时机


参考[接口概述](../interface/interface.md)，AimRT在运行时有三个主要的阶段：
- Initialize阶段
- Start阶段
- Shutdown阶段

一些接口只能在其中一些阶段里被调用。后续文档中，如果不对接口做特殊说明，则默认该接口在所有阶段都可以被调用。


## 大部分接口的实际表现需要根据部署运行配置而定

在逻辑实现阶段，开发者只需要知道此接口在抽象意义上代表什么功能即可，至于在实际运行时的表现，则要根据部署运行配置而定，在逻辑开发阶段也不应该关心太多。

例如，开发者可以使用`log`接口打印一行日志，但这个日志最终打印到文件里还是控制台上，则需要根据运行时配置而定，开发者在写业务逻辑时不需要关心。


## 接口层中的协程

AimRT中为执行器、RPC等功能提供了原生的异步回调形式的接口，同时也基于C++20协程和[C++ executors提案](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html)当前的一个实现库[libunifex](https://github.com/facebookexperimental/libunifex)，为使用者提供了一套协程形式的接口。

关于协程接口的基本用法，将在执行器、RPC等功能具体章节进行简单介绍。关于C++20协程以及libunifex库的进阶用法，请参考[C++20协程官方文档](https://en.cppreference.com/w/cpp/language/coroutines)和[libunifex主页](https://github.com/facebookexperimental/libunifex)。

请注意，协程功能是一个AimRT框架中的一个可选项，如果使用者不想使用协程的方式，也仍然能够通过其他形式的接口使用AimRT框架的所有基础能力。


## 接口层中的协议

在**aimrt::interface::aimrt_module_cpp_interface**这个CMake Target中，是不包含任何特定的协议类型的。在AimRT中，通过`aimrt_type_support_base_t`类来定义一种数据类型，其中定义了一种数据类型应该实现的基本接口，包括名称、创建/销毁、序列化/反序列化等。开发者可以通过实现它们来自定义数据类型，也可以直接使用AimRT官方支持的两种数据类型：
- **Protobuf**，需CMake引用**aimrt::interface::aimrt_module_protobuf_interface**；
- **ROS2 Message**，需CMake引用**aimrt::interface::aimrt_module_ros2_interface**；

一般在Channel或RPC功能中需要使用到具体的协议类型，具体的协议使用方式（包括代码生成、接口使用等）请参考Channel或RPC功能的文档章节。
