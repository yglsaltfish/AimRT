
# 运行时接口-Cpp版本


[TOC]

## 简介

&emsp;&emsp;如果说[Module接口](./cpp_module.md)主要是让用户开发具体的业务逻辑，产出物是一个个`Module`类，那么本文档所介绍的**运行时接口**则是让用户决定如何部署、集成、运行这些包含业务逻辑的`Module`。

&emsp;&emsp;AimRT为CPP版本的Module提供了两种部署集成方式：**App模式**和**Pkg模式**，前者是开发者在自己的Main函数中注册/创建各个模块，编译时直接将模块逻辑编译进主程序；后者则是使用AimRT提供的**aimrt_main**可执行程序，在运行时根据配置文件加载动态库形式的`Pkg`，导入其中的`Module`。

&emsp;&emsp;无论采用哪种方式都不影响业务逻辑，且两种方式可以共存，也可以比较简单的进行切换，实际采用哪种方式需要根据具体场景进行判断。


## App模式

&emsp;&emsp;开发者直接引用CMake Target：**aimrt::runtime::core**，然后即可使用[core/aimrt_core.h](../../../src/runtime/core/aimrt_core.h)文件中的`aimrt::runtime::core::AimRTCore`类，在App模式下需要使用的核心接口如下：

```cpp
namespace aimrt::runtime::core {

// AimRT 核心运行时类
class AimRTCore {
 public:
  // 启动选项
  struct Options {
    std::string cfg_file_path;  // 配置文件路径

    bool dump_cfg_file = false;      // 是否需要dump实际配置文件
    std::string dump_cfg_file_path;  // dump配置文件的路径
  };

 public:
  // 初始化框架
  void Initialize(const Options& options);

  // 启动框架。调用此方法后会阻塞当前线程，直到调用了Shutdown方法
  void Start();

  // 停止
  void Shutdown();

  // ...

  // 获取模块管理句柄
  module::ModuleManager& GetModuleManager();

  // ...
};

}  // namespace aimrt::runtime::core
```

&emsp;&emsp;接口使用说明如下：
- `void Initialize(const Options& options)`：用于初始化框架。
  - 接收一个`AimRTCore::Options`作为初始化参数。其中最重要的项是`cfg_file_path`，用于设置配置文件路径。关于配置文件的详细说明，请参考[配置](cfg.md)。
  - 如果初始化失败，会抛出一个异常。
- `void Start()`：启动框架。
  - 如果启动失败，会抛出一个异常。
  - 必须在Initialize方法之后调用，否则行为未定义。
  - 如果启动成功，会阻塞当前线程，并将当前线程作为本`AimRTCore`实例的主线程。
- `std::future<void> AsyncStart()`：异步启动框架。
  - 如果启动失败，会抛出一个异常。
  - 必须在Initialize方法之后调用，否则行为未定义。
  - 如果启动成功，会返回一个`std::future<void>`句柄，在外部可以调用该句柄的`wait`方法阻塞等待结束。
  - 该方法会在内部新启动一个线程作为本`AimRTCore`实例的主线程。
- `void Shutdown()`：停止框架。
  - 可以在任意线程、任意阶段调用此方法，也可以调用任意次数。
  - 调用此方法后，`Start`方法将在执行完主线程中的所有任务后，退出阻塞。但需要注意，有时候业务会阻塞住主线程中的任务，导致`Start`方法无法退出阻塞。这种情况下意味着无法优雅结束整个框架，需要强制kill。


&emsp;&emsp;开发者可以在自己的Main函数中创建一个`AimRTCore`实例，然后调用其`GetModuleManager`方法获取`ModuleManager`句柄，通过它注册或创建自己的业务模块。开发者需要依次调用`AimRTCore`实例的`Initialize`、`Start`/`AsyncStart`方法，并可以自己捕获`Ctrl-C`信号来调用`Shutdown`方法，以实现`AimRTCore`实例的优雅退出。

&emsp;&emsp;`GetModuleManager`方法返回的`ModuleManager`句柄可以用来注册或创建模块，App模式下需要使用其提供的`RegisterModule`接口或`CreateModule`接口：
```cpp
namespace aimrt::runtime::core::module {

class ModuleManager {
 public:
  // 注册一个模块
  void RegisterModule(const aimrt_module_base_t* module);

  // 创建一个模块
  const aimrt_core_base_t* CreateModule(std::string_view module_name);
};

}  // namespace aimrt::runtime::core::module
```

&emsp;&emsp;`RegisterModule`和`CreateModule`代表了App模式下编写逻辑的两种方式：**注册模块**与**创建模块**，前者仍然需要编写一个`Module`类，后者则更加自由。

### 注册模块


&emsp;&emsp;通过`RegisterModule`可以注册一个标准模块。开发者需要先实现一个包含`Initialize`、`Start`等方法的`Module`，然后在`AimRTCore`实例调用`Initialize`方法之前将该`Module`注册到`AimRTCore`实例中。在此方式下仍然有一个比较清晰的`Module`边界。


&emsp;&emsp;以下是一个简单的例子，开发者需要编写的`main.cc`文件如下：
```cpp
#include <csignal>

#include "core/aimrt_core.h"
#include "aimrt_module_cpp_interface/module_base.h"

// 全局的指针，用于信号处理
AimRTCore* global_core_ptr_ = nullptr;

// 信号处理函数
void SignalHandler(int sig) {
  if (global_core_ptr_ && (sig == SIGINT || sig == SIGTERM)) {
    // 接收到ctrl-c信号后停止框架
    global_core_ptr_->Shutdown();
    return;
  }
  raise(sig);
};

// 一个简单的模块
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

// 主函数
int32_t main(int32_t argc, char** argv) {
  // 注册ctrl-c信号
  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);

  // 创建AimRTCore实例
  AimRTCore core;
  global_core_ptr_ = &core;

  // 注册模块
  HelloWorldModule helloworld_module;
  core.GetModuleManager().RegisterModule(helloworld_module.NativeHandle());

  // 初始化框架，传入配置文件地址
  AimRTCore::Options options;
  options.cfg_file_path = "path/to/cfg/xxx_cfg.yaml";
  core.Initialize(options);

  // 启动框架，启动成功则阻塞在这里
  core.Start();

  // 停止框架
  core.Shutdown();

  return 0;
}
```

&emsp;&emsp;编译上面示例的`main.cc`，直接启动编译后的可执行文件即可运行进程，按下`ctrl-c`后即可停止进程。


### 创建模块

&emsp;&emsp;在`AimRTCore`实例调用`Initialize`方法之后，通过`CreateModule`可以创建一个模块，并返回一个`CoreRef`句柄，开发者可以直接基于此句柄调用一些框架的方法，比如RPC或者Log等。但这些方法仍然需要满足在[模块接口](./cpp_module.md)文档中所定义的、只能在特定阶段执行等相关规定。在此方式下没有一个比较清晰的`Module`边界，一般仅用于快速做一些小工具。

&emsp;&emsp;以下是一个简单的例子，开发者需要编写的`main.cc`文件如下：
```cpp
#include "core/aimrt_core.h"

#include "aimrt_module_cpp_interface/core.h"
#include "aimrt_module_protobuf_interface/channel/protobuf_channel.h"

#include "event.pb.h"

// 主函数
int32_t main(int32_t argc, char** argv) {
  // 创建AimRTCore实例
  AimRTCore core;

  // 初始化框架，进入Initialize阶段，传入配置文件地址
  AimRTCore::Options options;
  options.cfg_file_path = "path/to/cfg/xxx_cfg.yaml";
  core.Initialize(options);

  // 创建模块
  aimrt::CoreRef core_handle(core.GetModuleManager().CreateModule("HelloWorldModule"));

  // 注册一个publisher
  auto publisher = core_handle.GetChannelHandle().GetPublisher("test_topic");
  aimrt::channel::RegisterPublishType<ExampleEventMsg>(publisher);

  // 启动框架，进入Start阶段
  auto fu = core.AsyncStart();

  // 发布一个消息
  ExampleEventMsg msg;
  msg.set_msg("example msg");
  aimrt::channel::Publish(publisher, msg);

  // 等待一段时间
  std::this_thread::sleep_for(std::chrono::seconds(5));

  // 停止框架
  core.Shutdown();

  // 等待框架停止运行
  fu.wait();

  return 0;
}
```

&emsp;&emsp;编译上面示例的`main.cc`，直接启动编译后的可执行文件即可运行进程，该进程将在发布一个消息后，等待一段时间并退出。


## Pkg模式

&emsp;&emsp;开发者可以引用CMake Target：**aimrt::interface::aimrt_pkg_c_interface**，在其中的头文件[aimrt_pkg_c_interface/pkg_main.h](../../../src/interface/aimrt_pkg_c_interface/pkg_main.h)中，定义了4个要实现的接口：

```cpp
#ifdef __cplusplus
extern "C" {
#endif

// 获取当前动态库中有多少Module
size_t AimRTDynlibGetModuleNum();

// 获取当前动态库中Module名称列表
const aimrt_string_view_t* AimRTDynlibGetModuleNameList();

// 创建指定名称的Module
const aimrt_module_base_t* AimRTDynlibCreateModule(aimrt_string_view_t module_name);

// 销毁Module
void AimRTDynlibDestroyModule(const aimrt_module_base_t* module_ptr);

#ifdef __cplusplus
}
#endif
```

&emsp;&emsp;通过这4个接口，AimRT框架运行时可以从Pkg动态库中获取想要的模块。开发者需要在一个`C/CPP`文件中实现这些接口来创建一个Pkg。这些接口是纯C形式的，但如果开发者使用C++，也可以使用[aimrt_pkg_c_interface/pkg_macro.h](../../../src/interface/aimrt_pkg_c_interface/pkg_macro.h)文件中的一个简单的宏来封装这些细节，用户只需要实现一个内容类型为`std::tuple<std::string_view, std::function<aimrt::ModuleBase*()>>`的静态数组即可，在其中表明模块的名称和模块创建方法。

&emsp;&emsp;以下是一个简单的示例，开发者需要链接自己的模块lib，然后编写如下的`pkg_main.cc`文件：

```cpp
#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "bar_module.h"
#include "foo_module.h"

static std::tuple<std::string_view, std::function<aimrt::ModuleBase*()>> aimrt_module_register_array[]{
    {"FooModule", []() -> aimrt::ModuleBase* { return new FooModule(); }},
    {"BarModule", []() -> aimrt::ModuleBase* { return new BarModule(); }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
```

&emsp;&emsp;将上面的示例`pkg_main.cc`编译为动态库后，即可使用AimRT提供的**aimrt_main**可执行程序启动进程，通过配置中指定的路径来加载Pkg动态库。示例配置如下：
```yaml
aimrt:
  module: # 模块配置
    pkgs: # 要加载的动态库配置
      - path: /path/to/your/pkg/libxxx_pkg.so # so/dll地址
        disable_module: [] # 可选配置，此动态库中要屏蔽的模块名称。默认加载全部模块
```

&emsp;&emsp;有了配置文件之后，通过以下示例命令启动AimRT进程，按下`ctrl-c`后即可停止进程：
```shell
./aimrt_main --cfg_file_path=/path/to/your/cfg/xxx_cfg.yaml
```



## AimRT运行时的启动参数

&emsp;&emsp;AimRT官方提供**aimrt_main**可执行程序接收一些参数作为AimRT运行时的初始化参数，这些参数功能如下：

|  参数项  | 对应的AimRTCore::Options成员  | 类型 | 默认值 |作用 | 示例 |
|  ----  | ----  | ----  | ----  | ----  | ----  |
| cfg_file_path  | cfg_file_path | string | "" | 配置文件路径。 |  --cfg_file_path=/path/to/your/xxx_cfg.yaml |
| dump_cfg_file  | dump_cfg_file | bool | false | 是否Dump配置文件。<br>如果未指定dump_cfg_file_path参数，则直接通过日志打印。 |  --dump_cfg_file=true |
| dump_cfg_file_path  | dump_cfg_file_path | string | "" | Dump配置文件的路径。 |  --dump_cfg_file_path=/path/to/your/xxx_dump_cfg.yaml |
| register_signal  | - | bool | true | 是否注册sigint和sigterm信号 |  --register_signal=true |


&emsp;&emsp;开发者也可以使用`./aimrt_main --help`命令查看这些参数功能。


