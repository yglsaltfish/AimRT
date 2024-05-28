
# Module接口-Cpp版本

[TOC]

## 简介

&emsp;&emsp;AimRT为逻辑实现阶段的`Module`开发提供了一套CPP接口层，CMake Target名称为 **aimrt::interface::aimrt_module_cpp_interface**，代码见[aimrt_module_cpp_interface](https://code.agibot.com/agibot_aima/aimrt/-/tree/main/src/interface/aimrt_module_cpp_interface)，使用者在开发`Module`时只需要链接这个接口层库即可，可以与AimRT的实现细节相隔离。此接口层库的依赖只有两个：
- [fmt](https://github.com/fmtlib/fmt)：用于日志。如果使用C++20的format，则可以去掉这个依赖。
- [libunifex](https://github.com/facebookexperimental/libunifex)：用于将异步逻辑封装为协程。可以选用。


## 一些通用性说明

### 接口中的引用类型

- 大部分句柄是一种引用类型，类型命名一般以`Ref`结尾。
- 这种引用类型一般比较轻量，拷贝传递不会有较大开销。
- 这种引用类型一般都提供了一个`operator bool()`的重载来判断引用是否有效。
- 调用这种引用类型中提供的接口时，如果引用为空，会抛出一个异常。

### AimRT生命周期以及接口调用时机


&emsp;&emsp;参考[接口概述](interface.md)，AimRT在运行时有三个主要的阶段：Initialize阶段、Start阶段、Shutdown阶段，一些接口只能在其中一些阶段里被调用。本文档中，如果不对接口做特殊说明，则默认该接口在所有阶段都可以被调用。


### 大部分接口的实际表现需要根据部署运行配置而定

&emsp;&emsp;在逻辑实现阶段，开发者只需要知道此接口在抽象意义上代表什么功能即可，至于在实际运行时的表现，则要根据部署运行配置而定，在逻辑开发阶段也不应该关心太多。例如，开发者可以使用log接口打印一行日志，但这个日志最终打印到文件里还是控制台上，则需要根据运行时配置而定，开发者在写业务逻辑时不需要关心。


### 接口层中的协程

&emsp;&emsp;AimRT中为执行器、RPC等功能提供了原生的异步回调形式的接口，同时也基于C++20协程和[C++ executors提案](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html)当前的一个实现库[libunifex](https://github.com/facebookexperimental/libunifex)，为使用者提供了一套协程形式的接口。C++ executors提案预计将于C++26时被加入C++标准中，届时可能会提供选项将libunifex更换为标准库的实现。

&emsp;&emsp;AimRT中协程接口的代码位置：[aimrt_module_cpp_interface/co](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/co)。关于协程接口的基本用法，将在执行器、RPC等功能具体章节进行简单介绍。关于C++20协程以及libunifex库的进阶用法，请参考[C++20协程的官方文档页面](https://en.cppreference.com/w/cpp/language/coroutines)和[libunifex的官方github页面](https://github.com/facebookexperimental/libunifex)。

&emsp;&emsp;请注意，协程功能是一个AimRT框架中的一个可选项，如果使用者不想使用协程的方式，也仍然能够通过异步回调类型的接口使用AimRT框架的所有基础能力。


### 接口层中的协议

&emsp;&emsp;最纯粹的AimRT-CPP-Module接口，也就是**aimrt::interface::aimrt_module_cpp_interface**这个CMake Target，是不包含任何特定的协议类型的。在AimRT中，通过[aimrt_module_c_interface/util/type_support_base.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_c_interface/util/type_support_base.h)文件中的`aimrt_type_support_base_t`类来定义一种数据类型，其中定义了一个数据类型的应该实现的基本接口，包括名称、创建/销毁、序列化/反序列化等。使用者可以通过实现这些接口来自定义一种数据类型，也可以直接使用AimRT官方支持的两种数据类型：**Protobuf**和**ROS2 Message**。如果要使用它们，需要分别引用对应的CMake Target：
- 使用**Protobuf**类型需要引用的CMake Target：**aimrt::interface::aimrt_module_protobuf_interface**
- 使用**ROS2 Message**类型需要引用的CMake Target：**aimrt::interface::aimrt_module_ros2_interface**

&emsp;&emsp;一般在Channel或RPC功能中需要使用到具体的协议类型，所以具体的协议使用方式（包括代码生成、接口使用等）请参考Channel或RPC功能的对应文档章节。


## ModuleBase：模块基类

&emsp;&emsp;相关文件链接：[aimrt_module_cpp_interface/module_base.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/module_base.h)

&emsp;&emsp;参考示例：[helloworld_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/tree/main/src/examples/cpp/helloworld/module/helloworld_module/helloworld_module.cc)

&emsp;&emsp;所有的业务模块，都需要继承`aimrt::ModuleBase`基类，它定义了业务模块所需要实现的几个接口，具体接口如下：

```cpp
namespace aimrt {

class ModuleBase {
 public:
  // 获取模块信息
  virtual ModuleInfo Info() const = 0;

  // 初始化
  virtual bool Initialize(CoreRef core) = 0;

  // 开始
  virtual bool Start() = 0;

  // 关闭
  virtual void Shutdown() = 0;
};

}  // namespace aimrt
```

&emsp;&emsp;其中`aimrt::ModuleInfo`结构体声明如下：
```cpp
namespace aimrt {

struct ModuleInfo {
  std::string_view name;  // 必须项

  uint32_t major_version = 0;  // 可选
  uint32_t minor_version = 0;  // 可选
  uint32_t patch_version = 0;  // 可选
  uint32_t build_version = 0;  // 可选

  std::string_view author;       // 可选
  std::string_view description;  // 可选
};

}  // namespace aimrt
```

&emsp;&emsp;关于这些模块要实现虚接口，说明如下：
- `ModuleInfo Info()`：用于AimRT框架获取模块信息，包括模块名称、模块版本等。
  - AimRT框架会在加载模块时调用此接口，读取模块信息。
  - `ModuleInfo`结构中除`name`是必须项，其余都是可选项。
  - 如果模块在其中抛了异常，等效于返回一个空ModuleInfo。
- `bool Initialize(CoreRef core)`：用于初始化模块。
  - AimRT框架在Initialize阶段，依次调用各模块的Initialize方法。
  - AimRT框架保证在主线程中调用模块的Initialize方法，模块不应阻塞Initialize方法太久。
  - AimRT框架在调用模块Initialize方法时，会传入一个CoreRef句柄，模块可以存储此句柄，并在后续通过它调用框架的功能。
  - 在AimRT框架调用模块的Initialize方法之前，保证所有的组件（例如配置、日志等）都已经完成Initialize，但还未Start。
  - 如果模块在Initialize方法中抛了异常，等效于返回false。
  - 如果有任何模块在AimRT框架调用其Initialize方法时返回了false，则整个AimRT框架会Initialize失败。
- `bool Start()`：用于启动模块。
  - AimRT框架在Start阶段依次调用各模块的Start方法。
  - AimRT框架保证在主线程中调用模块的Start方法，模块不应阻塞Start方法太久。
  - 在AimRT框架调用模块的Start方法之前，保证所有的组件（例如配置、日志等）都已经进入Start阶段。
  - 如果模块在Start方法中抛了异常，等效于返回了false。
  - 如果有任何模块在AimRT框架调用其Start方法时返回了false，则整个AimRT框架会Start失败。
- `void Shutdown()`：用于停止模块，一般用于整个进程的优雅退出。
  - AimRT框架在Shutdown阶段依次调用各个模块的Shutdown方法。
  - AimRT框架保证在主线程中调用模块的Shutdown方法，模块不应阻塞Shutdown方法太久。
  - AimRT框架可能在任何阶段直接进入Shutdown阶段。
  - 如果模块在Shutdown方法中抛了异常，框架会catch住并直接返回。
  - 在AimRT框架调用模块的Shutdown方法之后，各个组件（例如配置、日志等）才会Shutdown。


&emsp;&emsp;以下是一个简单的示例，实现了一个最基础的HelloWorld模块：
```cpp
#include "aimrt_module_cpp_interface/module_base.h"

class HelloWorldModule : public aimrt::ModuleBase {
 public:
  HelloWorldModule() = default;
  ~HelloWorldModule() override = default;

  ModuleInfo Info() const override {
    return ModuleInfo{.name = "HelloWorldModule"};
  }

  bool Initialize(aimrt::CoreRef core) override { return true; }

  bool Start() override { return true; }

  void Shutdown() override {}
};
```

## CoreRef：框架句柄

&emsp;&emsp;相关文件链接：[aimrt_module_cpp_interface/core.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/core.h)

&emsp;&emsp;参考示例：[helloworld_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/tree/main/src/examples/cpp/helloworld/module/helloworld_module/helloworld_module.cc)

&emsp;&emsp;在模块的`Initialize`方法中，AimRT框架会传入一个`aimrt::CoreRef`句柄，模块通过该句柄的一些接口调用框架的功能。`aimrt::CoreRef`中提供的核心接口如下：

```cpp
namespace aimrt {

class CoreRef {
 public:
  // 获取所属模块的信息
  ModuleInfo Info() const;

  // 获取配置句柄
  configurator::ConfiguratorRef GetConfigurator() const;

  // 获取内存分配器句柄
  allocator::AllocatorRef GetAllocator() const;

  // 获取执行器管理句柄
  executor::ExecutorManagerRef GetExecutorManager() const;

  // 获取日志句柄
  logger::LoggerRef GetLogger() const;

  // 获取Rpc句柄
  rpc::RpcHandleRef GetRpcHandle() const;

  // 获取Channel句柄
  channel::ChannelHandleRef GetChannelHandle() const;

  // 获取参数句柄
  parameter::ParameterHandleRef GetParameterHandle() const;
};

}  // namespace aimrt
```


&emsp;&emsp;关于`aimrt::CoreRef`的使用注意点如下：
- AimRT框架会为每个模块生成一个专属`CoreRef`句柄，以实现资源隔离、监控等方面的功能。模块可以通过`CoreRef::Info`接口获取其所属的模块的信息。
- 模块通过`CoreRef`中的接口获取对应组件的句柄，并通过它们来调用相关功能。

&emsp;&emsp;一个简单的示例如下：
```cpp
bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  // 获取日志句柄
  auto logger = core.GetLogger();

  // 使用日志句柄打印日志
  AIMRT_HL_INFO(logger, "This is a test log");

  return true;
}
```

## configurator::ConfiguratorRef：配置句柄

&emsp;&emsp;相关文件链接：[aimrt_module_cpp_interface/configurator/configurator.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/configurator/configurator.h)

&emsp;&emsp;参考示例：[helloworld_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/tree/main/src/examples/cpp/helloworld/module/helloworld_module/helloworld_module.cc)

&emsp;&emsp;模块可以通过调用`CoreRef`句柄的`GetConfigurator()`接口，获取`aimrt::configurator::ConfiguratorRef`句柄，通过其使用一些配置相关的功能。其提供的核心接口如下：

```cpp
namespace aimrt::configurator {

class ConfiguratorRef {
 public:
  // 获取模块配置文件路径
  std::string_view GetConfigFilePath() const;
};

}  // namespace aimrt::configurator
```

&emsp;&emsp;使用注意点如下：
- `std::string_view GetConfigFilePath()`接口：用于获取模块配置文件的路径。
  - 请注意，此接口仅返回一个模块配置文件的路径，模块开发者需要自己读取配置文件并解析。
  - 这个接口具体会返回什么样的路径，请参考部署运行阶段[配置](./cfg.md)文档中的`aimrt.module`章节。


&emsp;&emsp;一个简单的使用示例如下：
```cpp
#include "yaml-cpp/yaml.h"

bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  // 获取配置句柄
  auto configurator = core.GetConfigurator();

  // 获取配置路径
  std::string_view cfg_file_path = configurator.GetConfigFilePath();

  // 根据用户实际使用的文件格式来解析配置文件。本例中基于yaml来解析
  YAML::Node cfg_node = YAML::LoadFile(std::string(cfg_file_path));

  // ...

  return true;
}
```

## executor::ExecutorManagerRef：执行器句柄

### AimRT CPP接口层中的执行器句柄

&emsp;&emsp;相关文件链接：
- [aimrt_module_cpp_interface/executor/executor_manager.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/executor/executor_manager.h)
- [aimrt_module_cpp_interface/executor/executor.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/executor/executor.h)

&emsp;&emsp;参考示例：[executor_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/executor/module/executor_module/executor_module.cc)

&emsp;&emsp;执行器`Executor`是一个很早就有的概念，它表示一个可以执行逻辑代码的抽象概念，一个执行器可以是一个线程池、可以是一个协程/纤程，可以是CPU、GPU、甚至是远端的一个服务器。我们平常写的最简单的代码也有一个默认的执行器：主线程。一般来说，执行器都会有类似这样的一个接口：
```cpp
void Execute(std::function<void()>);
```

&emsp;&emsp;这个接口表示，可以将一个类似于`std::function<void()>`的任务闭包投递到指定执行器中去执行。这个任务在何时何地执行则依赖于具体执行器的实现。C++标准库中的std::thread就是一个典型的执行器，它的构造函数接受传入一个`std::function<void()>`任务闭包，并将该任务放在一个新的线程中执行。


&emsp;&emsp;在AimRT中，模块可以通过调用`CoreRef`句柄的`GetExecutorManager()`接口，获取`aimrt::configurator::ExecutorManagerRef`句柄，其中提供了一个简单的获取Executor的接口：

```cpp
namespace aimrt::executor {

class ExecutorManagerRef {
 public:
  // 获取执行器句柄
  ExecutorRef GetExecutor(std::string_view executor_name) const;
};

}  // namespace aimrt::executor
```

&emsp;&emsp;使用者可以调用`ExecutorManagerRef`中的`GetExecutor`方法，获取指定名称的`aimrt::configurator::ExecutorRef`句柄，以调用执行器相关功能。`ExecutorRef`的核心接口如下：

```cpp
namespace aimrt::executor {

class ExecutorRef {
 public:
  // 类型
  std::string_view Type() const;

  // 名称  
  std::string_view Name() const;

  // 是否线程安全
  bool ThreadSafe() const;

  // 判断当前是否在本执行器中执行
  bool IsInCurrentExecutor() const;

  // 是否支持按时间调度
  bool SupportTimerSchedule() const;

  // 执行一个任务
  void Execute(Task&& task) const;

  // 获取本执行器体系下的时间
  std::chrono::system_clock::time_point Now() const;

  // 在某个时间点执行一个任务
  void ExecuteAt(std::chrono::system_clock::time_point tp, Task&& task) const;

  // 在某个时间后执行一个任务
  void ExecuteAfter(std::chrono::nanoseconds dt, Task&& task) const;
};

}  // namespace aimrt::executor
```


&emsp;&emsp;AimRT中的执行器有一些固有属性，这些固有属性大部分跟**执行器类型**相关，在运行过程中不会改变。这些固有属性包括：
- **执行器类型**：一个字符串字段，标识执行器在运行时的类型。
  - 在一个AimRT进程中，会存在多种类型的执行器，AimRT官方提供了几种执行器，插件也可以提供新类型的执行器。
  - 具体的执行器类型以及特性请参考部署环节的`executor`配置章节。
  - 在逻辑开发过程中，不应太关注实际运行时的执行器类型，只需根据抽象的执行器接口去实现业务逻辑。
- **执行器名称**：一个字符串字段，标识执行器在运行时的名称。
  - 在一个AimRT进程中，名称唯一标识了一个执行器。
  - 所有的执行器实例的名称都在运行时通过配置来决定，具体请参考部署环节的`executor`配置章节。
  - 可以通过`ExecutorManagerRef`的`GetExecutor`方法，获取指定名称的执行器。
- **线程安全性**：一个bool值，标识了本执行器是否是线程安全的。
  - 通常和执行器类型相关。
  - 线程安全的执行器可以保证投递到其中的任务不会同时运行。反之则不能保证。
- **是否支持按时间调度**：一个bool值，标识了本执行器是否支持按时间调度的接口，也就是`ExecuteAt`、`ExecuteAfter`接口。
  - 如果本执行器不支持按时间调度，则调用`ExecuteAt`、`ExecuteAfter`接口时会抛出一个异常。



&emsp;&emsp;关于`ExecutorRef`接口的详细使用说明如下：
- `std::string_view Type()`：获取执行器的类型。
- `std::string_view Name()`：获取执行器的名称。
- `bool ThreadSafe()`：返回本执行器是否是线程安全的。
- `bool IsInCurrentExecutor()`：判断调用此函数时是否在本执行器中。
  - 注意：如果返回true，则当前环境一定在本执行器中；如果返回false，则当前环境有可能不在本执行器中，也有可能在。
- `bool SupportTimerSchedule()`：返回本执行器是否支持按时间调度的接口，也就是`ExecuteAt`、`ExecuteAfter`接口。
- `void Execute(Task&& task)`：将一个任务投递到本执行器中，并在调度后立即执行。
  - 参数`Task`简单的视为一个满足`std::function<void()>`签名的任务闭包。
  - 此接口可以在Initialize/Start/Shutdown阶段调用，但执行器在Start阶段后才开始执行，因此在Start阶段之前调用此接口只能将任务投递到执行器的任务队列中而不会执行，等到Start之后才能开始执行任务。
- `std::chrono::system_clock::time_point Now()`：获取本执行器体系下的时间。
  - 对于一般的执行器来说，此处返回的都是`std::chrono::system_clock::now()`的结果。
  - 有一些带时间调速功能的特殊执行器，此处可能会返回经过处理的时间。
- `void ExecuteAt(std::chrono::system_clock::time_point tp, Task&& task)`：在某个时间点执行一个任务。
  - 第一个参数-时间点，以本执行器的时间体系为准。
  - 参数`Task`简单的视为一个满足`std::function<void()>`签名的任务闭包。
  - 如果本执行器不支持按时间调度，则调用此接口时会抛出一个异常。
  - 此接口可以在Initialize/Start/Shutdown阶段调用，但执行器在Start阶段后才开始执行，因此在Start阶段之前调用此接口只能将任务投递到执行器的任务队列中而不会执行，等到Start之后才能开始执行任务。
- `void ExecuteAfter(std::chrono::nanoseconds dt, Task&& task)`：在某个时间后执行一个任务。
  - 第一个参数-时间段，以本执行器的时间体系为准。
  - 参数`Task`简单的视为一个满足`std::function<void()>`签名的任务闭包。
  - 如果本执行器不支持按时间调度，则调用此接口时会抛出一个异常。
  - 此接口可以在Initialize/Start/Shutdown阶段调用，但执行器在Start阶段后才开始执行，因此在Start阶段之前调用此接口只能将任务投递到执行器的任务队列中而不会执行，等到Start之后才能开始执行任务。


&emsp;&emsp;以下是一个简单的使用示例，演示了如何获取一个执行器句柄，并将一个简单的任务投递到该执行器中执行：
```cpp
#include "aimrt_module_cpp_interface/module_base.h"

class HelloWorldModule : public aimrt::ModuleBase {
 public:
  bool Initialize(aimrt::CoreRef core) override {
    core_ = core;

    return true;
  }

  bool Start() override {
    // 获取名为 work_executor 的执行器句柄
    auto work_executor = core_.GetExecutorManager().GetExecutor("work_executor");

    // 检查执行器是否存在
    AIMRT_CHECK_ERROR_THROW(work_executor, "Can not get work_executor");

    // 将一个任务投递到执行器中执行
    work_executor.Execute([this]() {
      AIMRT_INFO("This is a simple task");
    });
  }

  // ...
 private:
  aimrt::CoreRef core_;
};
```

&emsp;&emsp;如果是一个线程安全的执行器，那么投递到其中的任务不需要加锁即可保证线程安全，示例如下：
```cpp
#include "aimrt_module_cpp_interface/module_base.h"

class HelloWorldModule : public aimrt::ModuleBase {
 public:
  bool Initialize(aimrt::CoreRef core) override {
    core_ = core;

    return true;
  }

  bool Start() override {
    // 获取名为 thread_safe_executor 的执行器句柄
    auto thread_safe_executor = core_.GetExecutorManager().GetExecutor("thread_safe_executor");

    // 检查执行器是否存在，并要求执行器是线程安全的
    AIMRT_CHECK_ERROR_THROW(thread_safe_executor && thread_safe_executor.ThreadSafe(),
                            "Can not get thread_safe_executor");

    // 将一些任务投递到执行器中执行
    uint32_t n = 0;
    for (uint32_t ii = 0; ii < 10000; ++ii) {
      thread_safe_executor_.Execute([&n]() {
        n++;
      });
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    AIMRT_INFO("Value of n is {}", n);
  }

  // ...
 private:
  aimrt::CoreRef core_;
};
```

&emsp;&emsp;以下这个示例则演示了如何使用Time Schedule接口，来实现定时循环：
```cpp
#include "aimrt_module_cpp_interface/module_base.h"

class HelloWorldModule : public aimrt::ModuleBase {
 public:
  bool Initialize(aimrt::CoreRef core) override {
    core_ = core;

    // 获取名为 time_schedule_executor 的执行器句柄
    auto time_schedule_executor_ = core_.GetExecutorManager().GetExecutor("time_schedule_executor");

    // 检查执行器是否存在，并要求执行器是支持time schedule接口的
    AIMRT_CHECK_ERROR_THROW(time_schedule_executor_ && time_schedule_executor_.SupportTimerSchedule(),
                            "Can not get time_schedule_executor");

    return true;
  }

  // 在该任务中再次定时执行自身任务，实现循环
  void ExecutorModule::TimeScheduleDemo() {
    if (!run_flag_) return;

    AIMRT_INFO("Loop count : {}", loop_count_++);

    time_schedule_executor_.ExecuteAfter(
        std::chrono::seconds(1),
        std::bind(&ExecutorModule::TimeScheduleDemo, this));
  }

  bool Start() override {
    // 开启循环
    TimeScheduleDemo();
  }

  // 需要在shutdown时将loop循环停掉
  void ExecutorModule::Shutdown() {
    run_flag_ = false;

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  // ...

 private:
  aimrt::CoreRef core_;

  bool run_flag_ = true;
  uint32_t loop_count_ = 0;
  aimrt::executor::ExecutorRef time_schedule_executor_;
};
```



### 执行器与协程接口


&emsp;&emsp;相关文件链接：
- [aimrt_module_cpp_interface/co/aimrt_context.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/co/aimrt_context.h)
- [aimrt_module_cpp_interface/co/async_scope.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/co/async_scope.h)
- [aimrt_module_cpp_interface/co/inline_scheduler.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/co/inline_scheduler.h)
- [aimrt_module_cpp_interface/co/on.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/co/on.h)
- [aimrt_module_cpp_interface/co/schedule.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/co/schedule.h)
- [aimrt_module_cpp_interface/co/sync_wait.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/co/sync_wait.h)
- [aimrt_module_cpp_interface/co/task.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/co/task.h)

&emsp;&emsp;参考示例：[executor_co_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/executor/module/executor_co_module/executor_co_module.cc)

&emsp;&emsp;AimRT框架中，为执行器封装了基于C++20协程和`libunifex`库的一个协程形式接口。关于协程和`libunifex`库的详细使用方式，请参考[libunifex官方文档](https://github.com/facebookexperimental/libunifex)。在AimRT框架中的[aimrt_module_cpp_interface/co/aimrt_context.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/co/aimrt_context.h)文件中，提供了一个比较重要的类：`aimrt::co::AimRTScheduler`，可以由`aimrt::executor::ExecutorRef`句柄构造。这个类将原生的AimRT执行器句柄封装成协程形式的接口句柄，其中的核心接口如下：

```cpp
namespace aimrt::co {

// 对应AimRT框架中的一个 ExecutorRef
class AimRTScheduler {
 public:
  // 由 ExecutorRef 构造
  explicit AimRTScheduler(executor::ExecutorRef executor_ref = {}) noexcept;
};

// 辅助类，对应AimRT框架中的一个 ExecutorManagerRef
class AimRTContext {
 public:
  // 由 ExecutorManagerRef 构造
  explicit AimRTContext(executor::ExecutorManagerRef executor_manager_ref = {}) noexcept;

  // 从执行器名称直接获取对应执行器的 AimRTScheduler 句柄
  AimRTScheduler GetScheduler(std::string_view executor_name) const;
};

}  // namespace aimrt::co
```


&emsp;&emsp;有了`AimRTScheduler`句柄，就可以使用`aimrt::co`命名空间下的一系列协程工具了。以下是一个简单的使用示例，演示了如何启动一个协程，并在协程中调度到指定执行器中执行任务：
```cpp
#include "aimrt_module_cpp_interface/co/async_scope.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/schedule.h"
#include "aimrt_module_cpp_interface/module_base.h"

class HelloWorldModule : public aimrt::ModuleBase {
 public:
  bool Initialize(aimrt::CoreRef core) override {
    core_ = core;

    // 获取名为 work_executor_1 的执行器句柄
    work_executor_1_ = core_.GetExecutorManager().GetExecutor("work_executor_1");
    AIMRT_CHECK_ERROR_THROW(work_executor_1_, "Can not get work_executor_1");

    // 获取名为 work_executor_2 的执行器句柄
    work_executor_2_ = core_.GetExecutorManager().GetExecutor("work_executor_2");
    AIMRT_CHECK_ERROR_THROW(work_executor_2_, "Can not get work_executor_2");

    return true;
  }

  bool Start() override {
    // 启动一个协程，使用当前执行器（当前的Start方法，是在主线程中运行）来初始执行该协程
    scope_.spawn(co::On(co::InlineScheduler(), MyTask()));

    return true;
  }

  aimrt::co::Task<void> MyTask() {
    // 在初始执行器中执行此行代码，此例中是主线程
    AIMRT_INFO("Now run in init executor");

    // 将executor句柄封装为协程所需要的scheduler句柄
    auto work_executor_1_scheduler = co::AimRTScheduler(work_executor_1_);

    // 调度到 work_executor_1_ 执行器中
    co_await aimrt::co::Schedule(work_executor_1_scheduler);

    // 在 work_executor_1_ 执行器中执行此行代码
    AIMRT_INFO("Now run in work_executor_1_");

    // 将executor句柄封装为协程所需要的scheduler句柄
    auto work_executor_2_scheduler = co::AimRTScheduler(work_executor_2_);

    // 调度到 work_executor_2_ 执行器中
    co_await aimrt::co::Schedule(work_executor_2_scheduler);

    // 在 work_executor_2_ 执行器中执行此行代码
    AIMRT_INFO("Now run in work_executor_2_");

    co_return;
  }

  void ExecutorCoModule::Shutdown() {
    // 阻塞的等待scope中所有协程执行完毕
    co::SyncWait(scope_.complete());

    AIMRT_INFO("Shutdown succeeded.");
  }

 private:
  auto GetLogger() { return core_.GetLogger(); }

 private:
  aimrt::CoreRef core_;
  aimrt::co::AsyncScope scope_;

  aimrt::executor::ExecutorRef work_executor_1_;
  aimrt::executor::ExecutorRef work_executor_2_;
};
```

&emsp;&emsp;以下这个示例则演示了如何使用Time Schedule接口，基于协程来实现定时循环：
```cpp
#include "aimrt_module_cpp_interface/co/async_scope.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/co/inline_scheduler.h"
#include "aimrt_module_cpp_interface/co/on.h"
#include "aimrt_module_cpp_interface/co/schedule.h"
#include "aimrt_module_cpp_interface/module_base.h"

class HelloWorldModule : public aimrt::ModuleBase {
 public:
  bool Initialize(aimrt::CoreRef core) override {
    core_ = core;

    // 获取名为 time_schedule_executor 的执行器句柄
    auto time_scheduler = core_.GetExecutorManager().GetExecutor("time_schedule_executor");
    AIMRT_CHECK_ERROR_THROW(time_scheduler && time_schedule_executor.SupportTimerSchedule(),
                            "Can not get time_schedule_executor");

    return true;
  }

  bool Start() override {
    // 启动一个协程，使用当前执行器（当前的Start方法，是在主线程中运行）来初始执行该协程
    scope_.spawn(co::On(co::InlineScheduler(), MainLoop()));

    return true;
  }

  aimrt::co::Task<void> MainLoop() {
    // 获取 scheduler 句柄。在init时已经判过空了
    auto time_scheduler = ctx_.GetScheduler("time_schedule_executor");

    // 调度到 time_schedule_executor 执行器中
    co_await co::Schedule(time_scheduler);

    uint32_t count = 0;
    while (run_flag_) {
      count++;
      AIMRT_INFO("Loop count : {} -------------------------", count);

      // 在一定时间后再调度到 time_schedule_executor 执行器中。等效于非阻塞的sleep
      co_await co::ScheduleAfter(time_scheduler, std::chrono::seconds(1));
    }

    AIMRT_INFO("Exit loop.");

    co_return;
  }

  void ExecutorCoModule::Shutdown() {
    // 阻塞的等待scope中所有协程执行完毕
    co::SyncWait(scope_.complete());

    AIMRT_INFO("Shutdown succeeded.");
  }

 private:
  auto GetLogger() { return core_.GetLogger(); }

 private:
  aimrt::CoreRef core_;
  aimrt::co::AsyncScope scope_;
  std::atomic_bool run_flag_ = true;

  co::AimRTContext ctx_;
};
```

## logger::LoggerRef：日志句柄

### 独立的日志组件

&emsp;&emsp;相关文件链接：[util/log_util.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/common/util/log_util.h)


&emsp;&emsp;在AimRT框架中，有一个独立的通用日志组件，属于**aimrt::common::util**这个CMake Target，只需要`#include "util/log_util.h"`即可独立于接口层使用。其中提供了一些基础的日志宏，这些日志宏需要在调用时传入一个日志句柄，来定义日志打印行为的具体表现。日志句柄以模板concept的形式定义，只要是类似于以下这个示例、包含`GetLogLevel`和`Log`两个接口的C++类的实例都可以作为日志句柄：
```cpp
class YourLogger {
 public:
  uint32_t GetLogLevel() const {
    // ...
  }

  void Log(uint32_t lvl, uint32_t line, uint32_t column,
           const char* file_name, const char* function_name,
           const char* log_data, size_t log_data_size) const {
    // ...
  }

};
```

&emsp;&emsp;其中，日志等级分为以下6档：
- Trace
- Debug
- Info
- Warn
- Error
- Fatal

&emsp;&emsp;在有了日志句柄之后，开发者可以直接基于日志句柄提供的`Log`方法打印日志，也可以使用提供的日志宏来更方便的打印日志。注意，提供的日志宏基于C++20 Format语法，关于C++20 Format语法的详细使用方式请参考[C++官方文档](https://en.cppreference.com/w/cpp/utility/format)。以下是一些使用示例：
```cpp
#include "util/log_util.h"

// 直接使用util/log_util.h中提供的一个简单版日志句柄，此日志句柄会同步的在控制台打印日志
auto lgr = aimrt::common::util::SimpleLogger();

uint32_t n = 42;
std::string s = "Hello world";

// 普通日志宏
AIMRT_HANDLE_LOG(lgr, aimrt::common::util::kLogLevelInfo, "This is a test log, n = {}, s = {}", n, s);
AIMRT_HL_TRACE(lgr, "This is a test trace log, n = {}, s = {}", n, s);
AIMRT_HL_DEBUG(lgr, "This is a test debug log, n = {}, s = {}", n, s);
AIMRT_HL_INFO(lgr, "This is a test info log, n = {}, s = {}", n, s);
AIMRT_HL_WARN(lgr, "This is a test warn log, n = {}, s = {}", n, s);
AIMRT_HL_ERROR(lgr, "This is a test error log, n = {}, s = {}", n, s);
AIMRT_HL_FATAL(lgr, "This is a test fatal log, n = {}, s = {}", n, s);

// 检查表达式，为false时才打印日志
AIMRT_HL_CHECK_ERROR(lgr, n == 41, "Expression is not right, n = {}", n);

// 打印日志并抛出异常
AIMRT_HL_ERROR_THROW(lgr, "This is a test error log, n = {}, s = {}", n, s);

// 检查表达式，为false时打印日志并抛出异常
AIMRT_HL_CHECK_TRACE_THROW(lgr, n == 41, "Expression is not right, n = {}", n);
```

&emsp;&emsp;此外，日志组件中还定义了一个默认的日志句柄获取接口`GetLogger()`，只要当前上下文中有`GetLogger()`这个方法，即可使用一些更简洁的日志宏，隐式的将`GetLogger()`方法返回的结果作为日志句柄，省略掉显式传递日志句柄这一步。示例如下：
```cpp
#include "util/log_util.h"

auto GetLogger() {
  return aimrt::common::util::SimpleLogger();
}

int Main() {
  uint32_t n = 42;
  std::string s = "Hello world";

  // 普通日志宏
  AIMRT_TRACE("This is a test trace log, n = {}, s = {}", n, s);
  AIMRT_DEBUG("This is a test debug log, n = {}, s = {}", n, s);
  AIMRT_INFO("This is a test info log, n = {}, s = {}", n, s);
  AIMRT_WARN("This is a test warn log, n = {}, s = {}", n, s);
  AIMRT_ERROR("This is a test error log, n = {}, s = {}", n, s);
  AIMRT_FATAL("This is a test fatal log, n = {}, s = {}", n, s);

  // 检查表达式，为false时才打印日志
  AIMRT_CHECK_ERROR(n == 41, "Expression is not right, n = {}", n);

  // 打印日志并抛出异常
  AIMRT_ERROR_THROW("This is a test error log, n = {}, s = {}", n, s);

  // 检查表达式，为false时打印日志并抛出异常
  AIMRT_CHECK_TRACE_THROW(n == 41, "Expression is not right, n = {}", n);

  // ...
}
```

### AimRT CPP接口层中的日志句柄

&emsp;&emsp;相关文件链接：[aimrt_module_cpp_interface/logger/logger.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/logger/logger.h)


&emsp;&emsp;在AimRT中，模块可以通过调用`CoreRef`句柄的`GetLogger()`接口，获取`aimrt::logger::LoggerRef`句柄，这是一个包含`GetLogLevel`和`Log`接口的类，满足上一节中对日志句柄的要求，可以直接作为日志宏的参数。其核心接口如下：
```cpp
namespace aimrt::logger {

class LoggerRef {
 public:
  // 获取日志等级
  uint32_t GetLogLevel() const;

  // 打印日志
  void Log(uint32_t lvl, uint32_t line, uint32_t column,
      const char* file_name, const char* function_name,
      const char* log_data, size_t log_data_size) const;
};

}  // namespace aimrt::logger
```

&emsp;&emsp;模块开发者可以直接参照以下示例的方式，使用分配给模块的日志句柄来打印日志：
```cpp
#include "aimrt_module_cpp_interface/module_base.h"

class HelloWorldModule : public aimrt::ModuleBase {
 public:
  bool Initialize(aimrt::CoreRef core) override {
    // 将分配给模块的日志句柄保存下来
    logger_ = core_.GetLogger();

    uint32_t n = 42;
    std::string s = "Hello world";

    // 普通日志宏，使用类内定义的GetLogger()方法的返回值作为日志句柄
    AIMRT_TRACE("This is a test trace log, n = {}, s = {}", n, s);
    AIMRT_DEBUG("This is a test debug log, n = {}, s = {}", n, s);
    AIMRT_INFO("This is a test info log, n = {}, s = {}", n, s);
    AIMRT_WARN("This is a test warn log, n = {}, s = {}", n, s);
    AIMRT_ERROR("This is a test error log, n = {}, s = {}", n, s);
    AIMRT_FATAL("This is a test fatal log, n = {}, s = {}", n, s);
  }

 private:
  // 在模块的作用域内定义一个GetLogger()方法
  auto GetLogger() { return logger_; }

 private:
  aimrt::logger::LoggerRef logger_;
};
```

## parameter::ParameterHandleRef：参数句柄


&emsp;&emsp;相关文件链接：[aimrt_module_cpp_interface/parameter/parameter_handle.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/parameter/parameter_handle.h)

&emsp;&emsp;参考示例：[parameter_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/parameter/module/parameter_module/parameter_module.cc)

&emsp;&emsp;AimRT中提供了一个简单的模块级kv参数功能，模块可以通过调用`CoreRef`句柄的`GetParameterHandle()`接口，获取`aimrt::parameter::ParameterHandleRef`句柄，来使用此功能。该句柄提供的核心接口如下：

```cpp
namespace aimrt::parameter {

class ParameterHandleRef {
 public:
  // 获取参数
  std::string GetParameter(std::string_view key) const;

  // 设置/更新参数
  void SetParameter(std::string_view key, std::string_view val) const;
};

}  // namespace aimrt::parameter
```

&emsp;&emsp;使用注意点如下：
- `std::string GetParameter(std::string_view key)`接口：用于获取参数。
  - 如果不存在key，则返回空字符串。
  - 该接口是线程安全的。
- `void SetParameter(std::string_view key, std::string_view val)`接口：用于设置/更新参数。
  - 如果不存在key，则新建一个key-val参数对。
  - 如果存在key，则更新key所对应的val值为最新值。
  - 该接口是线程安全的。
- 无论是设置参数还是获取参数，都是模块级别的，不同模块的参数互相独立、互不可见。


&emsp;&emsp;一个简单的使用示例如下：
```cpp
bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  // 获取参数句柄
  auto parameter_handle = core_.GetParameterHandle();

  std::string key = "test key";
  std::string val = "test val";

  // 设置参数
  parameter_handle_.SetParameter(key, val);

  // 获取参数
  std::string check_val = parameter_handle_.GetParameter(key);
  assert(check_val == val);

  return true;
}
```

&emsp;&emsp;除了通过本小节所介绍的CPP模块接口中的参数接口来设置/获取参数，使用者也可以通过parameter_plugin，实现通过RPC或者HTTP等方式来设置/获取参数的功能。具体请参考parameter_plugin的文档章节。


## allocator::AllocatorRef：内存分配器句柄

***TODO：Allocator功能还在完善中，暂不推荐直接使用***



## channel::ChannelHandleRef：Channel句柄

### Channel句柄概述

&emsp;&emsp;相关文件链接：
- [aimrt_module_cpp_interface/channel/channel_context.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/channel/channel_context.h)
- [aimrt_module_cpp_interface/channel/channel_handle.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/channel/channel_handle.h)


&emsp;&emsp;Protobuf Channel接口文件（CMake需引用**aimrt::interface::aimrt_module_protobuf_interface**）：
- [aimrt_module_protobuf_interface/channel/protobuf_channel.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_protobuf_interface/channel/protobuf_channel.h)


&emsp;&emsp;Ros2 Channel接口文件（CMake需引用**aimrt::interface::aimrt_module_ros2_interface**）：
- [aimrt_module_ros2_interface/channel/ros2_channel.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_ros2_interface/channel/ros2_channel.h)


&emsp;&emsp;参考示例：
- [protobuf_channel](https://code.agibot.com/agibot_aima/aimrt/-/tree/main/src/examples/cpp/protobuf_channel)
- [ros2_channel](https://code.agibot.com/agibot_aima/aimrt/-/tree/main/src/examples/cpp/ros2_channel)

&emsp;&emsp;AimRT中，模块可以通过调用`CoreRef`句柄的`GetChannelHandle()`接口，获取`aimrt::channel::ChannelHandleRef`句柄，来使用Channel功能。其提供的核心接口如下：
```cpp
namespace aimrt::channel {

class ChannelHandleRef {
 public:
  // 获取发布句柄
  PublisherRef GetPublisher(std::string_view topic) const;

  // 获取订阅句柄
  SubscriberRef GetSubscriber(std::string_view topic) const;

  // 获取Context管理句柄
  ContextManagerRef GetContextManager() const;
};

}  // namespace aimrt::channel
```

&emsp;&emsp;使用者可以调用`ChannelHandleRef`中的`GetPublisher`方法和`GetSubscriber`方法，获取**指定Topic名称**的`aimrt::channel::PublisherRef`句柄和`aimrt::channel::SubscriberRef`句柄，分别用于Channel发布和订阅。关于这两个方法，有一些使用时的须知：
  - 这两个接口是线程安全的。
  - 这两个接口可以在`Initialize`阶段和`Start`阶段使用。


&emsp;&emsp;这两个句柄提供了一个与具体协议类型无关的Api接口，但除非开发者想要使用自定义的消息类型，才需要直接调用它们提供的接口。AimRT官方支持两种协议类型：**Protobuf**和**Ros2 Message**，并提供了这两种协议类型的Channel接口封装。这两套Channel接口除了协议类型不同，其他的Api风格都一致，开发者一般直接使用这两套与协议类型绑定的Channel接口即可。


***TODO: CTX这部分的功能还在开发中，文档后续再补充***

&emsp;&emsp;使用者还可以调用`ChannelHandleRef`中的`GetContextManager`方法，获取一个`ContextManagerRef`句柄，它提供了一个方法来创建CTX，用于在发布、订阅Channel消息时传递一些额外的信息：

```cpp
namespace aimrt::channel {

class ContextManagerRef {
 public:
  // 创建一个CTX智能指针
  ContextSharedPtr NewContextSharedPtr() const;

  // 直接创建一个包含CTX智能指针的CTX引用
  ContextRef NewContextRef() const;
};

class ContextRef {
  // 获取设置的时间戳
  std::chrono::system_clock::time_point GetMsgTimestamp() const;

  // 设置一个时间戳
  void SetMsgTimestamp(std::chrono::system_clock::time_point deadline);

  // 获取一个K-V参数
  std::string_view GetMetaValue(std::string_view key) const;

  // 设置一个K-V参数
  void SetMetaValue(std::string_view key, std::string_view val) 
};

}  // namespace aimrt::channel
```


### 消息类型

&emsp;&emsp;一般来说，协议都是使用一种与具体的编程语言无关的`IDL`(Interface description language)描述，然后由某种工具转换为各个语言的代码。此处简要介绍一下几种`IDL`如何转换为Cpp代码，进阶的使用方式请参考对应IDL的官网。

#### Protobuf

&emsp;&emsp;[Protobuf](https://protobuf.dev/)（Protocol Buffers）是一种由Google开发的用于序列化结构化数据的轻量级、高效的数据交换格式，是一种广泛使用的IDL。它类似于XML和JSON，但更为紧凑、快速、简单，且可扩展性强。

&emsp;&emsp;在使用时，开发者需要先定义一个`.proto`文件，在其中定义一个消息结构。例如`example.proto`：

```protobuf
syntax = "proto3";

message ExampleMsg {
  string msg = 1;
  int32 num = 2;
}
```

&emsp;&emsp;然后使用Protobuf官方提供的protoc工具进行转换，生成C++桩代码，例如：
```shell
protoc --cpp_out=. example.proto
```

&emsp;&emsp;这将生成`example.pb.h`和`example.pb.cc`文件，包含了根据定义的消息类型生成的C++类和方法。

&emsp;&emsp;请注意，以上这套原生的代码生成方式只是为了给开发者展示底层的原理，实际使用的话需要手动处理依赖和CMake封装等方面的问题，并不推荐在项目中直接使用。开发者可以直接使用AimRT在[ProtobufGenCode.cmake](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/cmake/ProtobufGenCode.cmake)文件中提供的两个CMake方法：
- `add_protobuf_gencode_target_for_proto_path`：为某个路径下的协议文件生成C++代码，参数如下：
  - **TARGET_NAME**：生成的CMake Target名称；
  - **PROTO_PATH**：协议存放目录；
  - **GENCODE_PATH**：生成的桩代码存放路径；
  - **DEP_PROTO_TARGETS**：依赖的Proto CMake Target；
  - **OPTIONS**：传递给protoc的其他参数；
- `add_protobuf_gencode_target_for_one_proto_file`：为单个协议文件生成C++代码；
  - **TARGET_NAME**：生成的CMake Target名称；
  - **PROTO_FILE**：单个协议文件的路径；
  - **GENCODE_PATH**：生成的桩代码存放路径；
  - **DEP_PROTO_TARGETS**：依赖的Proto CMake Target；
  - **OPTIONS**：传递给protoc的其他参数；


&emsp;&emsp;使用示例如下：
```cmake
# 为当前文件夹下所有.proto文件生成C++桩代码
add_protobuf_gencode_target_for_proto_path(
  TARGET_NAME example_pb_gencode
  PROTO_PATH ${CMAKE_CURRENT_SOURCE_DIR}
  GENCODE_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_library(my_namespace::example_pb_gencode ALIAS example_pb_gencode)
```

&emsp;&emsp;这样之后，只要链接`my_namespace::example_pb_gencode`这个CMake Target即可使用该协议。例如：
```cmake
target_link_libraries(my_lib PUBLIC my_namespace::example_pb_gencode)
```

#### ROS2 Message

&emsp;&emsp;ROS2 Message是一种用于在 ROS2 中进行通信和数据交换的结构化数据格式。在使用时，开发者需要先定义一个ROS2 Package，在其中定义一个`.msg`文件，比如`example.msg`：

```
int32   num
float32 num2
char    data
```

&emsp;&emsp;然后直接通过ROS2提供的CMake方法`rosidl_generate_interfaces`，为消息生成C++代码和CMake Target，例如：
```cmake
rosidl_generate_interfaces(example_msg_gencode
  "msg/example.msg"
)
```

&emsp;&emsp;在这之后就可以引用相关的CMake Target来使用生成的C++代码。详情请参考ROS2的官方文档和AimRT提供的Example。


### Pub接口

&emsp;&emsp;参考示例：
- protobuf_channel:[normal_publisher_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/protobuf_channel/module/normal_publisher_module/normal_publisher_module.cc)
- ros2_channel:[normal_publisher_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/ros2_channel/module/normal_publisher_module/normal_publisher_module.cc)


&emsp;&emsp;用户如果需要发布一个Msg，牵涉的接口主要有以下几个：
```cpp
namespace aimrt::channel {

// 注册要发布的消息类型
template <MsgType>
bool RegisterPublishType(PublisherRef publisher);

// 发布消息，带CTX
template <MsgType>
void Publish(PublisherRef publisher, aimrt::channel::ContextRef ctx_ref, const MsgType& msg);

// 发布消息，不带CTX
template <MsgType>
void Publish(PublisherRef publisher, const MsgType& msg);

}  // namespace aimrt::channel
```

&emsp;&emsp;用户需要两个步骤来实现逻辑层面的消息发布：
- Step1：使用`RegisterPublishType`方法注册协议类型；
  - 只能在`Initialize`阶段注册；
  - 消息类型`MsgType`作为一个模板参数传入；
  - 不允许在一个`PublisherRef`中重复注册同一种类型；
  - 如果注册失败，会返回false；
- Step2：使用`Publish`方法发布数据；
  - 只能在`Start`阶段之后发布数据；
  - 消息类型`MsgType`作为一个模板参数传入；
  - 有两种`Publish`接口，其中一种多一个CTX参数，用于向后端、下游传递一些额外信息。CTX的具体功能由Channel后端决定。


&emsp;&emsp;用户在逻辑层`Publish`一个消息后，特定的Channel后端将处理具体的消息发布请求，此时根据后端的表现，有可能会阻塞一段时间，因此`Publish`方法耗费的时间是未定义的。但一般来说，Channel后端都不会阻塞`Publish`方法太久，详细的信息请参考您使用的后端的文档。

### Sub接口

&emsp;&emsp;参考示例：
- protobuf_channel:[normal_subscriber_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/protobuf_channel/module/normal_subscriber_module/normal_subscriber_module.cc)
- ros2_channel:[normal_subscriber_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/ros2_channel/module/normal_subscriber_module/normal_subscriber_module.cc)


&emsp;&emsp;AimRT提供了两种订阅接口来订阅处理一种消息，一种是智能指针形式，另一种是协程形式，两者在逻辑上是等价的。另外在订阅时可以接收一个CTX作为参数，CTX内容也和具体的Channel后端相关：

```cpp
// 订阅，回调接收CTX和一个指针指针作为参数
template <MsgType>
bool Subscribe(
    SubscriberRef subscriber,
    aimrt::util::Function<void(aimrt::channel::ContextRef ctx_ref, const std::shared_ptr<const MsgType>&)>&& callback);

// 订阅，回调接收一个指针指针作为参数
template <MsgType>
bool Subscribe(
    SubscriberRef subscriber,
    aimrt::util::Function<void(const std::shared_ptr<const MsgType>&)>&& callback);

// 订阅，回调是协程形式，并接收一个CTX作为参数
template <MsgType>
bool SubscribeCo(
    SubscriberRef subscriber,
    aimrt::util::Function<co::Task<void>(aimrt::channel::ContextRef ctx_ref, const MsgType&)>&& callback);

// 订阅，回调是协程形式
template <MsgType>
bool SubscribeCo(
    SubscriberRef subscriber,
    aimrt::util::Function<co::Task<void>(const MsgType&)>&& callback);
```


&emsp;&emsp;注意：
- 只能在`Initialize`调用订阅接口；
- 消息类型`MsgType`作为一个模板参数传入；
- 不允许在一个`SubscriberRef`中重复订阅同一种类型；
- 如果订阅失败，会返回false；

&emsp;&emsp;此外还需要注意的是，由哪个执行器来执行订阅的回调跟具体的Channel后端实现有关，这个是运行阶段通过配置才能确定的，使用者在编写逻辑代码时不应有任何假设。一般来说，如果回调中的任务非常轻量，那就可以直接在回调里处理；但如果回调中的任务比较重，那最好调度到其他专门执行任务的执行器里处理。


## rpc::RpcHandleRef：RPC句柄


### RPC句柄概述

&emsp;&emsp;相关文件链接：
- [aimrt_module_cpp_interface/rpc/rpc_handle.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/rpc/rpc_handle.h)
- [aimrt_module_cpp_interface/rpc/rpc_context.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/rpc/rpc_context.h)
- [aimrt_module_cpp_interface/rpc/rpc_status.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/rpc/rpc_status.h)
- [aimrt_module_cpp_interface/rpc/rpc_filter.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/rpc/rpc_filter.h)


&emsp;&emsp;参考示例：
- [protobuf_rpc](https://code.agibot.com/agibot_aima/aimrt/-/tree/main/src/examples/cpp/protobuf_rpc)
- [ros2_rpc](https://code.agibot.com/agibot_aima/aimrt/-/tree/main/src/examples/cpp/ros2_rpc)

&emsp;&emsp;AimRT中，模块可以通过调用`CoreRef`句柄的`GetRpcHandle()`接口，获取`aimrt::rpc::RpcHandleRef`句柄。开发者在使用RPC功能时必须要按照一定的步骤，调用`aimrt::rpc::RpcHandleRef`中的几个核心接口：
- Client端：
  - 在`Initialize`阶段，调用**注册RPC Client方法**的接口；
  - 在`Start`阶段，调用**RPC Invoke**的接口，以实现RPC调用；
- Server端：
  - 在`Initialize`阶段，**注册RPC Server服务**的接口；

&emsp;&emsp;一般情况下，使用者不会直接使用`aimrt::rpc::RpcHandleRef`直接提供的那些接口，而是根据RPC IDL文件生成一些桩代码，对`aimrt::rpc::RpcHandleRef`句柄进行一些封装，然后在业务代码中使用这些桩代码提供的接口。AimRT官方支持两种协议IDL：**Protobuf**和**Ros2 Srv**，并提供了针对这两种协议IDL生成桩代码的工具。生成出来的RPC接口除了协议类型不同，其他的Api风格都一致。



***TODO: CTX这部分的功能文档待补充***

&emsp;&emsp;使用者还可以调用`RpcHandleRef`中的`NewContextSharedPtr`方法和`NewContextRef`，创建一个RPC CTX，用于在调用、处理RPC请求时传递一些额外的信息。


&emsp;&emsp;此外，在RPC调用或者RPC处理时，使用者还可以通过一个`status`变量获取RPC请求时框架的错误情况，其中最主要的是一个错误码字段，其枚举值可以参考[rpc_status_base.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_c_interface/rpc/rpc_status_base.h)文件中的定义。

### 协议类型

&emsp;&emsp;一般来说，协议都是使用一种与具体的编程语言无关的`IDL`(Interface description language)描述，然后由某种工具转换为各个语言的代码。对于RPC来说，这里需要两个步骤：
- 参考上一节`Channel`中的介绍，开发者需要先利用一些官方的工具为协议文件中的**消息类型**生成指定编程语言中的代码；
- 开发者需要使用AimRT提供的工具，为协议文件中**服务定义**生成指定编程语言中的代码；

#### Protobuf

&emsp;&emsp;[Protobuf](https://protobuf.dev/)（Protocol Buffers）是一种由Google开发的用于序列化结构化数据的轻量级、高效的数据交换格式，是一种广泛使用的IDL。它不仅能够描述消息结构，还提供了`service`语句来定义RPC服务。


&emsp;&emsp;在使用时，开发者需要先定义一个`.proto`文件，在其中定义一个RPC服务。例如`rpc.proto`：

```protobuf
syntax = "proto3";

message ExampleReq {
  string msg = 1;
  int32 num = 2;
}

message ExampleRsp {
  uint64 code = 1;
  string msg = 2;
}

service ExampleService {
  rpc ExampleFunc(ExampleReq) returns (ExampleRsp);
}
```

&emsp;&emsp;然后使用Protobuf官方提供的protoc工具进行转换，生成消息结构部分的C++桩代码，例如：
```shell
protoc --cpp_out=. rpc.proto
```

&emsp;&emsp;这将生成`rpc.pb.h`和`rpc.pb.cc`文件，包含了根据定义的消息类型生成的C++类和方法。

&emsp;&emsp;在这之后，还需要使用AimRT提供的protoc插件，生成服务定义部分的C++桩代码，例如：
```shell
protoc --aimrt_rpc_out=. --plugin=protoc-gen-aimrt_rpc=./protoc_plugin_py_gen_aimrt_rpc.py rpc.proto
```

&emsp;&emsp;这将生成`rpc.aimrt_rpc.pb.h`和`rpc.aimrt_rpc.pb.cc`文件，包含了根据定义的服务生成的C++类和方法。

&emsp;&emsp;请注意，以上这套原生的代码生成方式只是为了给开发者展示底层的原理，实际使用的话需要手动处理依赖和CMake封装等方面的问题，并不推荐在项目中直接使用。开发者可以直接使用AimRT在[ProtobufAimRTRpcGenCode.cmake](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/tools/protoc_plugin_cpp_gen_aimrt_rpc/ProtobufAimRTRpcGenCode.cmake)文件中提供的CMake方法：`add_protobuf_aimrt_rpc_gencode_target_for_proto_files`，该方法可以直接为某些proto文件生成C++代码，其参数如下：
- **TARGET_NAME**：生成的CMake Target名称；
- **PROTO_FILES**：协议文件的路径；
- **GENCODE_PATH**：生成的桩代码存放路径；
- **DEP_PROTO_TARGETS**：依赖的Proto CMake Target；
- **OPTIONS**：传递给protoc的其他参数；


&emsp;&emsp;使用示例如下：
```cmake
# 为当前文件夹下所有.proto文件生成消息结构C++桩代码
add_protobuf_gencode_target_for_proto_path(
  TARGET_NAME example_pb_gencode
  PROTO_PATH ${CMAKE_CURRENT_SOURCE_DIR}
  GENCODE_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_library(my_namespace::example_pb_gencode ALIAS example_pb_gencode)

# 为rpc.proto文件生成RPC服务C++桩代码。需要依赖example_pb_gencode
add_protobuf_aimrt_rpc_gencode_target_for_proto_files(
  TARGET_NAME example_rpc_aimrt_rpc_gencode
  PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto
  GENCODE_PATH ${CMAKE_CURRENT_BINARY_DIR}
  DEP_PROTO_TARGETS my_namespace::example_pb_gencode)
add_library(my_namespace::example_rpc_aimrt_rpc_gencode ALIAS example_rpc_aimrt_rpc_gencode)
```


&emsp;&emsp;这样之后，只要链接`my_namespace::example_rpc_aimrt_rpc_gencode`这个CMake Target即可使用该协议。例如：
```cmake
target_link_libraries(my_lib PUBLIC my_namespace::example_rpc_aimrt_rpc_gencode)
```


#### ROS2 Srv

&emsp;&emsp;ROS2 Srv是一种用于在 ROS2 中进行RPC定义的格式。在使用时，开发者需要先定义一个ROS2 Package，在其中定义一个`.srv`文件，比如`example.srv`：

```
byte[]  data
---
int64   code
```

&emsp;&emsp;其中，以`---`来分割Req和Rsp的定义。然后直接通过ROS2提供的CMake方法`rosidl_generate_interfaces`，为Req和Rsp消息生成C++代码和CMake Target，例如：
```cmake
rosidl_generate_interfaces(example_srv_gencode
  "srv/example.srv"
)
```

&emsp;&emsp;在这之后就可以引用相关的CMake Target来使用生成的Req和Rsp消息结构C++代码。详情请参考ROS2的官方文档和AimRT提供的Example。

&emsp;&emsp;在这之后，开发者还需要使用AimRT提供的Python脚本工具，生成服务定义部分的C++桩代码，例如：
```shell
python3 ARGS ./ros2_py_gen_aimrt_rpc.py --pkg_name=example_pkg --srv_file=./example.srv --output_path=./
```

&emsp;&emsp;这将生成`example.aimrt_rpc.srv.h`和`example.aimrt_rpc.srv.cc`文件，包含了根据定义的服务生成的C++类和方法。


&emsp;&emsp;请注意，以上这套原生的代码生成方式只是为了给开发者展示底层的原理，实际使用的话需要手动处理依赖和CMake封装等方面的问题，并不推荐在项目中直接使用。开发者可以直接使用AimRT在[Ros2AimRTRpcGenCode.cmake](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/tools/ros2_py_gen_aimrt_rpc/Ros2AimRTRpcGenCode.cmake)文件中提供的CMake方法：`add_ros2_aimrt_rpc_gencode_target_for_one_file`，该方法可以为单个srv文件生成RPC服务C++代码，其参数如下：
- **TARGET_NAME**：生成的CMake Target名称；
- **PACKAGE_NAME**：ROS2协议PKG的名称；
- **PROTO_FILE**：协议文件的路径；
- **GENCODE_PATH**：生成的桩代码存放路径；
- **DEP_PROTO_TARGETS**：依赖的协议 CMake Target；
- **OPTIONS**：传递给工具的其他参数；


&emsp;&emsp;使用示例如下：
```cmake
# 为example.srv文件生成RPC服务C++桩代码。需要依赖ROS2消息相关CMake Target，在此统一定义在${ROS2_EXAMPLE_CMAKE_TARGETS}变量内
add_ros2_aimrt_rpc_gencode_target_for_one_file(
  TARGET_NAME example_ros2_rpc_aimrt_rpc_gencode
  PACKAGE_NAME example_pkg
  PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/srv/example.srv
  GENCODE_PATH ${CMAKE_CURRENT_BINARY_DIR}
  DEP_PROTO_TARGETS
    rclcpp::rclcpp
    ${ROS2_EXAMPLE_CMAKE_TARGETS})
add_library(my_namespace::example_ros2_rpc_aimrt_rpc_gencode ALIAS example_ros2_rpc_aimrt_rpc_gencode)
```


&emsp;&emsp;这样之后，只要链接`my_namespace::example_ros2_rpc_aimrt_rpc_gencode`这个CMake Target即可使用该协议。例如：
```cmake
target_link_libraries(my_lib PUBLIC my_namespace::example_ros2_rpc_aimrt_rpc_gencode)
```


### Client接口

&emsp;&emsp;在AimRT RPC桩代码工具生成的代码里，如`rpc.aimrt_rpc.pb.h`或者`example.aimrt_rpc.srv.h`里，提供了三种类型的Client Proxy接口，开发者基于这些Proxy接口类来发起RPC调用：
- **同步型接口**：名称一般为`XXXSyncProxy`；
- **异步型接口**：名称一般为`XXXAsyncProxy`；
- **无栈协程型接口**：名称一般为`XXXCoProxy`；

&emsp;&emsp;这三种类型可以混合使用，开发者可以根据自身需求选用。

#### 同步型接口

&emsp;&emsp;参考示例：TODO



&emsp;&emsp;同步型接口在使用上最简单，但在运行效率上是最低的。它通过阻塞当前线程，等待RPC接口返回。一般可以在一些不要求性能的场合为了提高开发效率而使用这种方式，但不推荐在高性能要求的场景使用。

&emsp;&emsp;使用同步型接口发起RPC调用非常简单，一般分为以下几个步骤：
- Step0：引用桩代码头文件，例如`xxx.aimrt_rpc.pb.h`或者`xxx.aimrt_rpc.srv.h`，其中有同步接口的句柄`XXXSyncProxy`；
- Step1：在`Initialize`阶段调用`RegisterClientFunc`方法注册RPC Client；
- Step2：在`Start`阶段里某个业务函数里发起RPC调用：
  - Step2-1：创建一个`XXXSyncProxy`，构造参数是`aimrt::rpc::RpcHandleRef`。proxy非常轻量，可以随用随创建；
  - Step2-2：创建Req、Rsp，并填充Req内容；
  - Step2-3：【可选】创建ctx，设置超时等信息；
  - Step2-4：基于proxy，传入ctx、Req、Rsp，发起RPC调用，同步等待RPC调用结束，获取返回的status；
  - Step2-5：解析status和Rsp；



&emsp;&emsp;以下是一个简单的基于protobuf的示例，基于ROS2 Srv的语法也基本类似：
```cpp
bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  // Step1：在Initialize阶段调用RegisterClientFunc方法注册RPC Client
  bool ret = aimrt::protocols::example::RegisterExampleServiceClientFunc(core_.GetRpcHandle());
  AIMRT_CHECK_ERROR_THROW(ret, "Register client failed.");

  return true;
}

// 在Step2：在Start阶段里某个业务函数里发起RPC调用
void HelloWorldModule::Foo() {
  // Step2-1：创建一个proxy，构造参数是 aimrt::rpc::RpcHandleRef 。proxy非常轻量，可以随用随创建
  ExampleServiceSyncProxy proxy(core_.GetRpcHandle());

  // Step2-2：创建Req、Rsp，并填充Req内容
  ExampleReq req;
  ExampleRsp rsp;
  req.set_msg("hello world");

  // Step2-3：【可选】创建ctx，设置超时等信息
  auto ctx = proxy.NewContextRef();
  ctx.SetTimeout(std::chrono::seconds(3));

  // Step2-4：基于proxy，传入ctx、Req、Rsp，发起RPC调用，同步等待RPC调用结束，获取返回的status
  auto status = proxy.ExampleFunc(ctx, req, rsp);

  // Step2-5：解析status和Rsp
  if (status.OK()) {
    auto msg = rsp.msg();
    // ...
  } else {
    // ...
  }
}
```


#### 异步型接口

&emsp;&emsp;参考示例：TODO


&emsp;&emsp;异步型接口使用回调来返回异步结果，在性能上表现最好，但开发友好度是最低的，很容易陷入回调地狱。

&emsp;&emsp;使用异步型接口发起RPC调用一般分为以下几个步骤：
- Step0：引用桩代码头文件，例如`xxx.aimrt_rpc.pb.h`或者`xxx.aimrt_rpc.srv.h`，其中有异步接口的句柄`XXXAsyncProxy`；
- Step1：在`Initialize`阶段调用`RegisterClientFunc`方法注册RPC Client；
- Step2：在`Start`阶段里某个业务函数里发起RPC调用：
  - Step2-1：创建一个`XXXAsyncProxy`，构造参数是`aimrt::rpc::RpcHandleRef`。proxy非常轻量，可以随用随创建；
  - Step2-2：创建Req、Rsp，并填充Req内容；
  - Step2-3：【可选】创建ctx，设置超时等信息；
  - Step2-4：基于proxy，传入ctx、Req、Rsp和结果回调，发起RPC调用，并保证在整个调用周期里ctx、Req、Rsp都保持生存；
  - Step2-5：在回调函数中获取返回的status，解析status和Rsp；

&emsp;&emsp;前几个步骤与同步型接口基本一致，区别在于Step2-4需要使用异步回调的方式来获取结果。以下是一个简单的基于protobuf的示例，基于ROS2 Srv的语法也基本类似：
```cpp
bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  // Step1：在Initialize阶段调用RegisterClientFunc方法注册RPC Client
  bool ret = aimrt::protocols::example::RegisterExampleServiceClientFunc(core_.GetRpcHandle());
  AIMRT_CHECK_ERROR_THROW(ret, "Register client failed.");

  return true;
}

// 在Step2：在Start阶段里某个业务函数里发起RPC调用
void HelloWorldModule::Foo() {
  // Step2-1：创建一个proxy，构造参数是 aimrt::rpc::RpcHandleRef 。proxy非常轻量，可以随用随创建
  ExampleServiceAsyncProxy proxy(core_.GetRpcHandle());

  // Step2-2：创建Req、Rsp，并填充Req内容
  // 由于要保证req、rsp的生命周期比RPC调用的周期长，此处可以利用智能指针来实现
  auto req = std::make_shared<ExampleReq>();
  auto rsp = std::make_shared<ExampleRsp>();
  req->set_msg("hello world");

  // Step2-3：【可选】创建ctx，设置超时等信息
  // 由于要保证ctx的生命周期比RPC调用的周期长，此处可以利用智能指针来实现
  auto ctx_ptr = proxy.NewContextSharedPtr();
  aimrt::rpc::ContextRef ctx(ctx_ptr.get());
  ctx.SetTimeout(std::chrono::seconds(3));

  // Step2-4：基于proxy，传入ctx、Req、Rsp和结果回调，发起RPC调用，并保证在整个调用周期里ctx、Req、Rsp都保持生存
  proxy.GetBarData(
      ctx, *req, *rsp,
      [this, ctx_ptr, req, rsp](aimrt::rpc::Status status) {
        // Step2-5：在回调函数中获取返回的status，解析status和Rsp
        if (status.OK()) {
          auto msg = rsp->msg();
          // ...
        } else {
          // ...
        }
      });
}
```


#### 无栈协程型接口

&emsp;&emsp;参考示例：TODO

&emsp;&emsp;AimRT为RPC Client端提供了一套基于C++20协程和[C++ executors提案](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html)当前的一个实现库[libunifex](https://github.com/facebookexperimental/libunifex)来实现的一套无栈协程形式的接口。无栈协程接口在本质上是对异步型接口的封装，在性能上基本与异步型接口一致，但大大提升了开发友好度。

&emsp;&emsp;使用协程型接口发起RPC调用一般分为以下几个步骤：
- Step0：引用桩代码头文件，例如`xxx.aimrt_rpc.pb.h`或者`xxx.aimrt_rpc.srv.h`，其中有协程接口的句柄`XXXCoProxy`；
- Step1：在`Initialize`阶段调用`RegisterClientFunc`方法注册RPC Client；
- Step2：在`Start`阶段里某个业务协程里发起RPC调用：
  - Step2-1：创建一个`XXXCoProxy`，构造参数是`aimrt::rpc::RpcHandleRef`。proxy非常轻量，可以随用随创建；
  - Step2-2：创建Req、Rsp，并填充Req内容；
  - Step2-3：【可选】创建ctx，设置超时等信息；
  - Step2-4：基于proxy，传入ctx、Req、Rsp和结果回调，发起RPC调用，在协程中等待RPC调用结束，获取返回的status；
  - Step2-5：解析status和Rsp；


&emsp;&emsp;整个接口风格与同步型接口几乎一样，但必须要在协程中调用。以下是一个简单的基于protobuf的示例，基于ROS2 Srv的语法也基本类似：
```cpp
bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  // Step1：在Initialize阶段调用RegisterClientFunc方法注册RPC Client
  bool ret = aimrt::protocols::example::RegisterExampleServiceClientFunc(core_.GetRpcHandle());
  AIMRT_CHECK_ERROR_THROW(ret, "Register client failed.");

  return true;
}

// Step2：在`Start`阶段里某个业务协程里发起RPC调用
co::Task<void> HelloWorldModule::Foo() {
  // Step2-1：创建一个proxy，构造参数是 aimrt::rpc::RpcHandleRef 。proxy非常轻量，可以随用随创建
  ExampleServiceCoProxy proxy(core_.GetRpcHandle());

  // Step2-2：创建Req、Rsp，并填充Req内容
  ExampleReq req;
  ExampleRsp rsp;
  req.set_msg("hello world");

  // Step2-3：【可选】创建ctx，设置超时等信息
  auto ctx = proxy.NewContextRef();
  ctx.SetTimeout(std::chrono::seconds(3));

  // Step2-4：基于proxy，传入ctx、Req、Rsp和结果回调，发起RPC调用，在协程中等待RPC调用结束，获取返回的status；
  auto status = co_await proxy.ExampleFunc(ctx, req, rsp);

  // Step2-5：解析status和Rsp
  if (status.OK()) {
    auto msg = rsp.msg();
    // ...
  } else {
    // ...
  }
}
```

### Server接口

&emsp;&emsp;在AimRT RPC桩代码工具生成的代码里，如`rpc.aimrt_rpc.pb.h`或者`example.aimrt_rpc.srv.h`里，提供了三种类型的Service基类，开发者继承这些Service基类，实现其中的虚接口来提供实际的RPC服务：
- **同步型接口**：名称一般为`XXXSyncService`；
- **异步型接口**：名称一般为`XXXAsyncService`；
- **无栈协程型接口**：名称一般为`XXXCoService`；

&emsp;&emsp;在单个service内，这三种类型的不能混合使用，只能选择一种，开发者可以根据自身需求选用。


#### 同步型接口

&emsp;&emsp;参考示例：TODO

&emsp;&emsp;同步型接口在使用上最简单，但很多时候实现的service中需要请求下游，会有一些异步调用，这种情况下只能阻塞的等待下游调用完成，可能会造成运行效率上的降低。一般可以在处理一些简单的请求、不需要发起其他异步调用的场景下使用同步型接口。

&emsp;&emsp;使用同步型接口实现RPC服务，一般分为以下几个步骤：
- Step0：引用桩代码头文件，例如`xxx.aimrt_rpc.pb.h`或者`xxx.aimrt_rpc.srv.h`，其中有同步接口的Service基类`XXXSyncService`；
- Step1：开发者实现一个Impl类，继承`XXXSyncService`，并实现其中的虚接口；
  - Step1-1：解析Req，并填充Rsp；
  - Step1-2：返回`Status`；
- Step2：在`Initialize`阶段调用`RegisterService`方法注册RPC Service；



&emsp;&emsp;以下是一个简单的基于protobuf的示例，基于ROS2 Srv的语法也基本类似：
```cpp
// Step1：开发者实现一个Impl类，继承`XXXSyncService`，并实现其中的虚接口
class ExampleServiceSyncServiceImpl : public ExampleServiceSyncService {
 public:
  ExampleServiceSyncServiceImpl() = default;
  ~ExampleServiceSyncServiceImpl() override = default;

  aimrt::rpc::Status ExampleFunc(
      aimrt::rpc::ContextRef ctx, const ExampleReq& req, ExampleRsp& rsp) override {
    // Step1-1：解析Req，并填充Rsp
    rsp.set_msg("echo " + req.msg());

    // Step1-2：返回`Status`
    return aimrt::rpc::Status();
  }
};

bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  // Step2：在`Initialize`阶段调用`RegisterService`方法注册RPC Service
  service_ptr_ = std::make_shared<ExampleServiceSyncServiceImpl>();

  bool ret = core_.GetRpcHandle().RegisterService(service_ptr_.get());
  AIMRT_CHECK_ERROR_THROW(ret, "Register service failed.");

  return true;
}
```

#### 异步型接口

&emsp;&emsp;参考示例：TODO

&emsp;&emsp;异步型接口会传递一个回调给开发者，开发者在RPC处理完成后调用这个回调来传递最终处理结果。这种方式可以在RPC中发起其他异步调用，由于不会阻塞，因此性能表现通常最好，但通常会导致开发出的代码难以阅读和维护。

&emsp;&emsp;使用异步型接口实现RPC服务，一般分为以下几个步骤：
- Step0：引用桩代码头文件，例如`xxx.aimrt_rpc.pb.h`或者`xxx.aimrt_rpc.srv.h`，其中有异步接口的Service基类`XXXAsyncService`；
- Step1：开发者实现一个Impl类，继承`XXXAsyncService`，并实现其中的虚接口；
  - Step1-1：解析Req，并填充Rsp；
  - Step1-2：调用callback将`Status`传递回去；
- Step2：在`Initialize`阶段调用`RegisterService`方法注册RPC Service；


&emsp;&emsp;以下是一个简单的基于protobuf的示例，基于ROS2 Srv的语法也基本类似：
```cpp
// Step1：开发者实现一个Impl类，继承`XXXSyncService`，并实现其中的虚接口
class ExampleServiceAsyncServiceImpl : public ExampleServiceAsyncService {
 public:
  ExampleServiceAsyncServiceImpl() = default;
  ~ExampleServiceAsyncServiceImpl() override = default;

  void ExampleFunc(
      aimrt::rpc::ContextRef ctx, const ExampleReq& req, ExampleRsp& rsp,
      aimrt::util::Function<void(aimrt::rpc::Status)>&& callback) override {
    // Step1-1：解析Req，并填充Rsp
    rsp.set_msg("echo " + req.msg());

    // Step1-2：调用callback将`Status`传递回去；
    callback(aimrt::rpc::Status());
  }
};

bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  // Step2：在`Initialize`阶段调用`RegisterService`方法注册RPC Service
  service_ptr_ = std::make_shared<ExampleServiceAsyncServiceImpl>();

  bool ret = core_.GetRpcHandle().RegisterService(service_ptr_.get());
  AIMRT_CHECK_ERROR_THROW(ret, "Register service failed.");

  return true;
}
```


#### 无栈协程型接口

&emsp;&emsp;参考示例：TODO

&emsp;&emsp;与RPC Client端一样，在RPC Service端，AimRT也提供了一套基于C++20协程和[C++ executors提案](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html)当前的一个实现库[libunifex](https://github.com/facebookexperimental/libunifex)来实现的一套无栈协程形式的接口。无栈协程接口在本质上是对异步型接口的封装，在性能上基本与异步型接口一致，但大大提升了开发友好度。


&emsp;&emsp;使用协程型接口实现RPC服务，一般分为以下几个步骤：
- Step0：引用桩代码头文件，例如`xxx.aimrt_rpc.pb.h`或者`xxx.aimrt_rpc.srv.h`，其中有异步接口的Service基类`XXXCoService`；
- Step1：开发者实现一个Impl类，继承`XXXCoService`，并实现其中的虚接口；
  - Step1-1：解析Req，并填充Rsp；
  - Step1-2：使用co_return返回`Status`；
- Step2：在`Initialize`阶段调用`RegisterService`方法注册RPC Service；



&emsp;&emsp;整个接口风格与同步型接口几乎一样。以下是一个简单的基于protobuf的示例，基于ROS2 Srv的语法也基本类似：
```cpp
// Step1：开发者实现一个Impl类，继承`XXXSyncService`，并实现其中的虚接口
class ExampleServiceCoServiceImpl : public ExampleServiceCoService {
 public:
  ExampleServiceCoServiceImpl() = default;
  ~ExampleServiceCoServiceImpl() override = default;

  co::Task<aimrt::rpc::Status> ExampleFunc(
      aimrt::rpc::ContextRef ctx, const ExampleReq& req, ExampleRsp& rsp) override {
    // Step1-1：解析Req，并填充Rsp
    rsp.set_msg("echo " + req.msg());

    // Step1-2：使用co_return返回`Status`；
    co_return aimrt::rpc::Status();
  }
};

bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  // Step2：在`Initialize`阶段调用`RegisterService`方法注册RPC Service
  service_ptr_ = std::make_shared<ExampleServiceCoServiceImpl>();

  bool ret = core_.GetRpcHandle().RegisterService(service_ptr_.get());
  AIMRT_CHECK_ERROR_THROW(ret, "Register service failed.");

  return true;
}
```
