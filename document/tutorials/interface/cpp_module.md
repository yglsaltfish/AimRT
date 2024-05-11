
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
  - 这个接口具体会返回什么样的路径，请参考部署运行阶段的`configurator`配置章节。


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

## rpc::RpcHandleRef：RPC句柄

***TODO待完善***

## channel::ChannelHandleRef：Channel句柄

***TODO待完善***

