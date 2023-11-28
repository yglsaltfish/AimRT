
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

## 快速入门

### 使用Modern CMake构建项目

&emsp;&emsp;Modern CMake是目前主流C++项目的常用构建方式，其使用面向对象的构建思路，引入了Target、属性等概念，将包括依赖在内的各个参数信息全部封装起来，大大简化了依赖管理，使构建大型系统更有条理、更加轻松。


### 使用脚手架工具创建示例项目

&emsp;&emsp;参考以下配置内容：
```yaml
# 一些基本信息
base_info:
  project_name: my_prj
  build_mode_tags: ["EXAMPLE"] # 构建模式标签
  aimrt_import_options: # 引入aimrt时的一些选项
    AIMRT_BUILD_TESTS: "OFF"

# 两个协议
protocols:
  - name: my_proto
    type: protobuf
  - name: example_proto
    type: protobuf
    build_mode_tag: ["EXAMPLE"] #仅在EXAMPLE模式为true时构建。build_mode_tag未设置则表示默认在所有模式下都构建

# 三个模块
modules:
  - name: my_foo_module
  - name: my_bar_module
  - name: exmaple_module
    build_mode_tag: ["EXAMPLE"]

# 两个Pkg，一个包含两个模块，另一个只包含一个模块
pkgs:
  - name: pkg1
    modules:
      - name: my_foo_module
      - name: my_bar_module
  - name: pkg2
    build_mode_tag: ["EXAMPLE"]
    modules:
      - name: exmaple_module

# 两种部署模式
deploy_modes:
  - name: exmaple_mode
    build_mode_tag: ["EXAMPLE"]
    deploy_ins:
      - name: local_ins
        pkgs:
          - name: pkg1
            options:
              disable_module: ["my_bar_module"]
          - name: pkg2
      - name: remote_ins
        pkgs:
          - name: pkg1
            options:
              disable_module: ["my_foo_module"]
          - name: pkg2
  - name: my_deploy_mode
    deploy_ins:
      - name: local_ins
        pkgs:
          - name: pkg1
            options:
              disable_module: ["my_bar_module"]
      - name: remote_ins
        pkgs:
          - name: pkg1
            options:
              disable_module: ["my_foo_module"]

```

&emsp;&emsp;将以上内容保存为配置文件`test_prj.yaml`，然后执行以下命令即可生成一套基础示例工程：

```shell
aimrt_cli gen -p path/to/my_prj.yaml -o path/to/your_desire
```

&emsp;&emsp;生成之后，目录结构类似如下：

```
├── cmake/
├── src/
│   ├── CMakeLists.txt
│   ├── install/
│   ├── module/
│   │   ├── exmaple_module/
│   │   ├── my_bar_module/
│   │   └── my_foo_module/
│   ├── pkg/
│   │   ├── pkg1/
│   │   └── pkg2/
│   └── protocols/
│       ├── example_proto/
│       └── my_proto/
├── CMakeLists.txt
├── format.sh
├── README.md
├── build.sh
└── test.sh
```

&emsp;&emsp;该工程中包含3个模块、2个Pkg。具体关系可以参考配置文件中的注释。可以直接执行根路径下的`build.sh`编译项目。编译完成后可以直接执行`build/install/bin`路径下的相关启动脚本启动对应进程。


&emsp;&emsp;具体环境配置以及aimrt_cli工具介绍详见[aimrt_cli工具详解](./aimrt_cli_introduction.md)


### 实现一个简单的`Module`
&emsp;&emsp;通过以下代码可以基于C++实现一个简单的`Module`：
```cpp
#include "aimrt_module_cpp_interface/module_base.h"

class HelloWorldModule : public aimrt::ModuleBase {
 public:
  HelloWorldModule() = default;
  ~HelloWorldModule() override = default;

  // 模块基本信息
  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "HelloWorldModule"};
  }

  // 初始化，框架在加载模块后会调用模块的初始化方法，传入CoreRef，作为模块调用框架功能的句柄
  bool Initialize(aimrt::CoreRef core) noexcept override {
    core_ = core;
    AIMRT_HL_INFO(core_.GetLogger(), "Init succeeded.");
    return true;
  }
  
  // 启动模块，在初始化后框架会调用模块的Start方法
  bool Start() noexcept override {
    AIMRT_HL_INFO(core_.GetLogger(), "Start succeeded.");
    return true;
  }

  // 关闭，框架在结束运行时会调用模块的关闭方法
  void Shutdown() noexcept override {
    AIMRT_HL_INFO(core_.GetLogger(), "Shutdown succeeded.");
  }

 private:
  aimrt::CoreRef core_; // 模块应将初始化时传入的CoreRef存储下来
};
```

&emsp;&emsp;模块的`Initialize`方法和`Start`方法的区别在于：
- 框架会先调用各个模块`Initialize`方法，所有模块初始化完成后再调用各个模块的`Start`方法。
- RPC、Channel等功能中的注册函数只能在`Initialize`方法中被调用，而RPC的Invoke、Channel的Publish等运行时方法只能在`Initialize`方法后调用。

### 实现一个简单的`Pkg`
&emsp;&emsp;通过以下代码可以基于C++实现一个简单的`Pkg`：

```cpp
#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "helloworld_module/helloworld_module.h"

// 核心是创建一个<模块名称, 模块创建方法>的表
static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"HelloWorldModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::example_helloworld::helloworld_module::
               HelloWorldModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
```

### 启动一个`AimRT`进程

&emsp;&emsp;启动一个`AimRT`进程需要一个配置文件。框架使用yaml作为基本配置文件格式。参照如下yaml代码作为配置文件：
```yaml
aimrt:
  configurator:
    temp_cfg_path: ./cfg/tmp # 生成的临时模块配置文件存放路径
  log: # log配置
    core_lvl: TRACE # 内核日志等级，可选项：Trace/Debug/Info/Warn/Error/Fatal/Off，不区分大小写
    default_module_lvl: INFO # 模块默认日志等级
    backends: # 日志backends
      - type: console # 控制台日志
        options:
          color: true # 是否彩色打印
      - type: rotate_file # 文件日志
        options:
          path: ./log # 日志文件路径
          filename: example_helloworld.log # 日志文件名称
          max_file_size_m: 4 # 日志文件最大尺寸，单位m
          max_file_num: 10 # 最大日志文件数量，0代表无限
  module: # 模块配置
    pkgs: # 要加载的动态库配置
      - path: ./libhelloworld_pkg.so # so/dll地址
        disable_module: [] # 此动态库中要屏蔽的模块名称。默认全部加载
    modules: # 模块
      - name: HelloWorldModule # 模块Name接口返回的名称
        log_lvl: INFO # 模块日志级别
        # cfg_file_path: ./cfg/my_module_cfg.txt

# 模块自定义配置，框架会为每个模块生成临时配置文件，开发者通过Configurator接口获取该配置文件路径
HelloWorldModule:
  key1: val1
  key2: val2

```

&emsp;&emsp;这个配置文件中的其他内容将在后续章节中介绍。这里关注`aimrt.module`节点，这个节点配置了要加载的模块包动态库路径，以及各个模块的配置。我们在这里配置框架加载刚刚编译的`helloworld_pkg`模块包动态库，加载其中的`HelloWorldModule`模块。

&emsp;&emsp;将以上配置文件保存为`test_cfg.yaml`，并确保要加载的模块包处于正确的路径下，然后执行以下命令启动`AimRT`进程：
```shell
aimrt_main --cfg_file_path=/path/to/test_cfg.yaml
```

&emsp;&emsp;启动后即可通过日志观察模块运行情况。启动后按下`ctrl + c`即可停止进程。其他启动参数可以通过`aimrt_main --help`查看。


## 业务模块开发

### 业务模块生命周期

&emsp;&emsp;业务模块开发主要围绕模块的三个方法进行：
- `bool Initialize(aimrt::CoreRef core)`
- `bool Start()`
- `void Shutdown()`

&emsp;&emsp;框架会在初始化、启动、停止时调用所有模块的这三个方法，流程如下图所示：

（TODO）


&emsp;&emsp;其中，在初始化时，框架会给模块传入一个`aimrt::CoreRef`变量。该变量是一个引用，拷贝传递不会有较大开销。模块通过该变量的一些接口调用框架的功能。


### 使用配置功能

&emsp;&emsp;模块可以通过在初始化时传入的`aimrt::CoreRef`变量的`GetConfigurator()`接口，获取`aimrt::ConfiguratorRef`变量，并通过其`GetConfigFilePath()`方法获取模块配置文件的路径，模块可以依据此在初始化方法中自行读取配置文件：
```cpp
bool HelloWorldModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  try {
    aimrt::ConfiguratorRef configurator = core_.GetConfigurator();
    if (configurator) {
      // 根据返回的配置文件路径，使用相应方式读取模块配置文件。例如这里使用yaml-cpp打开该配置文件
      YAML::Node cfg_node = YAML::LoadFile(configurator.GetConfigFilePath());

      // ...
    }
  }

  return true;
}
```

&emsp;&emsp;注意：如果在框架根配置yaml文件的模块配置中指定了模块配置文件路径，则此处返回指定的文件路径。例如：
```yaml
aimrt:
  module: # 模块配置
    modules: # 模块
      - name: HelloWorldModule # 模块Name接口返回的名称
        cfg_file_path: ./cfg/my_module_cfg.txt
```
&emsp;&emsp;则此时`GetConfigFilePath()`方法返回值为`./cfg/my_module_cfg.txt`。该配置文件为用户自定义，不限制格式。

&emsp;&emsp;如果在框架根配置yaml文件中存在以模块名为名称的节点，则框架会为该模块生成一个临时yaml配置文件，并将模块配置写入到此临时文件中，此时`GetConfigFilePath()`方法返回值为这个临时配置文件的路径。例如框架的根配置yaml文件如下所示时：
```yaml
aimrt:
  configurator:
    temp_cfg_path: ./cfg/tmp # 生成的临时模块配置文件存放路径
  module: # 模块配置
    modules: # 模块
      - name: HelloWorldModule # 模块Name接口返回的名称

# 模块自定义配置，框架会为每个模块生成临时配置文件，开发者通过Configurator接口获取该配置文件路径
HelloWorldModule:
  key1: val1
  key2: val2
```
&emsp;&emsp;则此时`GetConfigFilePath()`方法将返回一个临时配置文件路径，该临时配置文件将位于`aimrt.configurator.temp_cfg_path`节点所配置的目录下，其中的内容如下：
```yaml
key1: val1
key2: val2
```

### 使用日志功能


&emsp;&emsp;模块可以使用初始化时传入的`aimrt::CoreRef`变量的`GetLogger()`方法获取日志句柄来打日志，不同模块拿到的日志句柄会根据配置有不同的表现。可以配合一些日志宏，使用C++20 format语法打日志：
```cpp
std::string s = "abc";
AIMRT_HL_TRACE(core_.GetLogger(), "test trace log, num: {}, s: {}", 123, s);
AIMRT_HL_DEBUG(core_.GetLogger(), "test debug log, num: {}, s: {}", 123, s);
AIMRT_HL_INFO(core_.GetLogger(), "test info log, num: {}, s: {}", 123, s);
AIMRT_HL_WARN(core_.GetLogger(), "test warn log, num: {}, s: {}", 123, s);
AIMRT_HL_ERROR(core_.GetLogger(), "test error log, num: {}, s: {}", 123, s);
AIMRT_HL_FATAL(core_.GetLogger(), "test fatal log, num: {}, s: {}", 123, s);
```

&emsp;&emsp;如果在当前上下文中定义了默认的日志句柄`GetLogger()`，则可以省略第一个参数，使用以下几个日志宏来打日志：
```cpp
// 定义有日志句柄方法
LoggerRef GetLogger();

// ...

std::string s = "abc";
AIMRT_TRACE("test trace log, num: {}, s: {}", 123, s);
AIMRT_DEBUG("test debug log, num: {}, s: {}", 123, s);
AIMRT_INFO("test info log, num: {}, s: {}", 123, s);
AIMRT_WARN("test warn log, num: {}, s: {}", 123, s);
AIMRT_ERROR("test error log, num: {}, s: {}", 123, s);
AIMRT_FATAL("test fatal log, num: {}, s: {}", 123, s);
```

&emsp;&emsp;开发者可以在模块内定义`GetLogger()`成员方法从而实现模块类内日志功能，也可以在特定命名空间内定义全局`GetLogger()`方法实现特定范围内的全局日志功能。

&emsp;&emsp;日志配置参考如下：
```yaml
aimrt:
  log: # log配置
    core_lvl: TRACE # 内核日志等级，可选项：Trace/Debug/Info/Warn/Error/Fatal/Off，不区分大小写
    default_module_lvl: TRACE # 模块默认日志等级
    writers: # 日志writers。此处只提供两种默认的，可以在代码中通过addwriter接口手动添加
      - type: console_writer # 控制台writer
        color: true # 是否彩色打印
      - type: rotate_file_writer # 文件writer
        path: ./log # 日志文件路径
        filename: normal_example.log # 日志文件名称
        max_file_size_m: 16 # 日志文件最大尺寸，单位m
        max_file_num: 10 # 最大日志文件数量，0代表无限
  module: # 模块配置
    modules: # 模块
      - name: HelloWorldModule # 模块Name接口返回的名称
        log_lvl: TRACE # 模块日志级别

```

&emsp;&emsp;在`aimrt.log`中可以配置框架的日志级别，以及默认的模块日志级别，同时提供几种日志后端供选择。在模块配置`aimrt.module`中，也可以为不同的模块配置不同的日志级别。可配置的日志级别参见以下列表：
- Trace
- Debug
- Info
- Warn
- Error
- Fatal
- Off


### 使用执行器


&emsp;&emsp;在框架启动时，可以配置一个或多个执行器。目前框架只支持线程型执行器，后续可能支持更多的执行器种类。例如框架使用以下配置时：
```yaml
aimrt:
  executor: # 执行器配置
    executors: # 当前先支持thread型/strand型，未来可根据加载的网络模块提供更多类型
      - name: work_thread_pool # 线程池
        type: asio_thread # 类型为asio实现的线程池
        thread_num: 4 # 线程数，不指定则默认单线程
      - name: my_single_thread # 单线程
        type: asio_thread # 类型为asio实现的线程池
```

&emsp;&emsp;当框架启动时，将创建两个执行器：
- 名称为`work_thread_pool`的、包含4个线程的线程池执行器
- 名称为`my_single_thread`的、只有一个线程的单线程执行器

&emsp;&emsp;模块可以通过在初始化时传入的`aimrt::CoreRef`变量的`GetExecutorManager()`方法，获取框架提供的执行器管理器句柄，类型为`ExecutorManagerRef`。可以使用它的`GetExecutor`方法获取具体的执行器句柄：
```cpp
bool HelloWorldModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  // 获取执行器句柄，参数为配置时的执行器名称
  ExecutorRef executor = core_.GetExecutorManager().GetExecutor("work_thread_pool");
  return true;
}
```

&emsp;&emsp;通过获取到的`ExecutorRef`指针，模块可以使用`Execute(task)`方法将任务投递到各个执行器中去执行：
```cpp
bool HelloWorldModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  // 获取执行器句柄，参数为配置时的执行器名称
  executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");

  // 将在work_thread_pool线程池中执行投递的任务
  executor_.Execute([logger = core_.GetLogger()]() {
    AIMRT_HL_TRACE(logger, "test execute");
  });

  return true;
}
```


### 使用Channel通信

&emsp;&emsp;Channel功能使模块之间可以订阅发布消息。参考[example_normal_channel](https://code.agibot.com/agibot-tech/aimrt/-/tree/main/src/examples/example_normal_channel)示例，我们使用`NormalPublisherModule`模块不断发布一个事件，让`NormalSubscriberModule`模块订阅这个事件。其中订阅和发布模块封装在两个Pkg中，如下所示：
- normal_channel_alpha_pkg
  - NormalPublisherModule
- normal_channel_beta_pkg
  - NormalSubscriberModule

&emsp;&emsp;此时核心目录结构如下：
```
+ src
  + install // 安装时需要的一些文件
    + bin // 启停脚本、配置文件等
      - cfg.yaml
  + module // 模块
    + normal_publisher_module // Publisher模块
      - CMakeLists.txt
      - normal_publisher_module.cc
      - normal_publisher_module.h
    + normal_subscriber_module // Subscriber模块
      - CMakeLists.txt
      - normal_subscriber_module.cc
      - normal_subscriber_module.h
  + pkg // 模块包
    + normal_channel_alpha_pkg // 模块包，编译后是一个动态库
      - CMakeLists.txt
      - pkg_main.cc
    + normal_channel_beta_pkg // 模块包，编译后是一个动态库
      - CMakeLists.txt
      - pkg_main.cc
  + protocols // 协议
    + example // event协议
      - CMakeLists.txt
      - event.proto
```

&emsp;&emsp;其中`src/protocols/example/event.proto`协议文件来定义事件消息的结构，我们可以使用Protobuf来定义。其代码参考如下：
```protobuf
syntax = "proto3";

package aimrt.protocols.example;

message ExampleEventMsg {
  string msg = 1;
  int32 num = 2;
}
```

&emsp;&emsp;然后在`src/protocols/example/CMakeLists.txt`脚本中为其生成桩代码CMake Target：
```cpp
add_protobuf_gencode_target_for_proto_path(
  TARGET_NAME example_pb_gencode
  PROTO_PATH ${CMAKE_CURRENT_SOURCE_DIR}
  GENCODE_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_library(my_namespace::example_pb_gencode ALIAS example_pb_gencode)
```

&emsp;&emsp;这样之后，无论是Publisher端还是Subscriber端，只要链接`my_namespace::example_pb_gencode`这个Target即可使用该协议。例如在作为Publisher端的`NormalPublisherModule`模块下，在其CMakeLists.txt中使用如下代码即可：
```cmake
target_link_libraries(normal_publisher_module PUBLIC my_namespace::example_pb_gencode)
```

&emsp;&emsp;引用之后，在作为Publish端的`NormalPublisherModule`模块中，需要先进行结构的注册，然后才能调用发布方法，示例代码如下：
```cpp
bool NormalPublisherModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  // 注册事件消息结构
  std::string topic_name = "test_topic";
  publisher_ = core_.GetChannel().GetPublisher(topic_name);
  aimrt::channel::RegisterPublishType<aimrt::protocols::example::ExampleEventMsg>(publisher_);

  // 获取要跑任务的执行器句柄
  executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");

  return true;
}

bool NormalPublisherModule::Start() noexcept {
  // 启动一个跑循环的协程
  scope_.spawn(MainLoop());
  return true;
}

void NormalPublisherModule::Shutdown() noexcept {
  // 模块Shutdown时需要结束协程
  run_flag_ = false;
  co::SyncWait(scope_.complete());
}

// 主循环
aimrt::co::Task<void> NormalPublisherModule::MainLoop() {
  try {
    aimrt::co::AimRTScheduler work_thread_pool_scheduler(executor_);

    while (run_flag_) {
      // 等待一段时间
      co_await aimrt::co::ScheduleAfter(
          work_thread_pool_scheduler, std::chrono::microseconds(1000));

      // 创建要发布的数据结构并填数据
      aimrt::protocols::example::ExampleEventMsg msg;
      msg.set_msg("hello!");

      // 发布接口
      aimrt::channel::Publish(publisher_, msg);
    }

  } catch (const std::exception& e) {
    // ...
  }

  co_return;
}
```

&emsp;&emsp;该示例代码中使用了协程作为逻辑流程的组织方式。实际业务使用时可以在任意地方调用Publish接口，不一定非要用协程。

&emsp;&emsp;在订阅端，事件处理的回调方法提供两种接口：
- 智能指针形式接口：
  ```cpp
  // 订阅回调函数
  void EventHandle(const std::shared_ptr<const aimrt::protocols::example::ExampleEventMsg>& data_ptr);

  // 订阅接口
  aimrt::channel::Subscribe<aimrt::protocols::example::ExampleEventMsg>(
        subscriber_, EventHandle);
  ```
- 协程形式接口：
  ```cpp
  // 订阅回调函数
  aimrt::co::Task<void> EventHandle(const aimrt::protocols::example::ExampleEventMsg& data);

  // 订阅接口
  aimrt::channel::SubscribeCo<aimrt::protocols::example::ExampleEventMsg>(
        subscriber_, EventHandle);
  ```

&emsp;&emsp;在作为Subscriber端的`NormalSubscriberModule`模块中，示例代码如下：

```cpp
bool NormalSubscriberModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  // 订阅事件
  std::string topic_name = "test_topic";
  subscriber_ = core_.GetChannel().GetSubscriber(topic_name);
  aimrt::channel::SubscribeCo<aimrt::protocols::example::ExampleEventMsg>(
        subscriber_, std::bind(&NormalSubscriberModule::EventHandle, this, std::placeholders::_1));

  return true;
}

// 事件处理方法
aimrt::co::Task<void> NormalSubscriberModule::EventHandle(
    const aimrt::protocols::example::ExampleEventMsg& data) {
  AIMRT_INFO("Get new pb event, data: {}", aimrt::Pb2CompactJson(data));

  co_return;
}
```

&emsp;&emsp;需要注意的是，无论Publish端的`RegisterPublishType`方法和Subscribe端的`Subscribe`/`SubscribeCo`方法，都必须在模块的`Initialize`方法内调用，不能在初始化之后再注册，否则会返回注册失败。


### 使用rpc通信

&emsp;&emsp;Rpc功能使模块之间可以调用/提供服务。参考[example_normal_rpc](https://code.agibot.com/agibot-tech/aimrt/-/tree/main/src/examples/example_normal_rpc)示例，我们使用`NormalRpcServerModule`模块作为服务端提供一个Rpc服务，让`NormalRpcClientModule`模块创建一个客户端调用这个服务。其中服务端模块和客户端模块封装在两个Pkg中，如下所示：
- normal_rpc_alpha_pkg
  - NormalRpcClientModule
- normal_rpc_beta_pkg
  - NormalRpcServerModule

&emsp;&emsp;此时核心目录结构如下：
```
+ src // 代码
  + install // 安装时需要的一些文件
    + bin // 启停脚本、配置文件等
      - cfg.yaml
  + module // 模块
    + normal_rpc_client_module // client模块
      - CMakeLists.txt
      - normal_rpc_client_module.cc
      - normal_rpc_client_module.h
    + normal_rpc_server_module // server模块
      - CMakeLists.txt
      - normal_rpc_server_module.cc
      - normal_rpc_server_module.h
      - global.cc
      - global.h
      - rpc_service.cc
      - rpc_service.h
  + pkg // 模块包
    + normal_rpc_alpha_pkg // 模块包，编译后是一个动态库
      - CMakeLists.txt
      - pkg_main.cc
    + normal_rpc_beta_pkg // 模块包，编译后是一个动态库
      - CMakeLists.txt
      - pkg_main.cc
  + protocols // 协议
    + example // rpc协议
      - CMakeLists.txt
      - rpc.proto
```

&emsp;&emsp;我们首先需要使用`src/protocols/example/rpc.proto`文件定义RPC协议，参照以下代码：
```protobuf
syntax = "proto3";

import "common.proto";

package aimrt.protocols.example;

message GetFooDataReq {
  string msg = 1;
}

message GetFooDataRsp {
  uint64 code = 1;
  string msg = 2;
  aimrt.protocols.example.ExampleFoo data = 3;
}

service ExampleService {
  rpc GetFooData(GetFooDataReq) returns (GetFooDataRsp);
}
```

&emsp;&emsp;然后在`src/proto/rpc_proto/CMakeLists.txt`脚本中为其生成桩代码target：
```cpp
add_protobuf_gencode_target_for_proto_path(
  TARGET_NAME example_pb_gencode
  PROTO_PATH ${CMAKE_CURRENT_SOURCE_DIR}
  GENCODE_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_library(my_namespace::example_pb_gencode ALIAS example_pb_gencode)

add_protobuf_aimrt_rpc_gencode_target_for_proto_files(
  TARGET_NAME example_aimrt_rpc_gencode
  PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto
  GENCODE_PATH ${CMAKE_CURRENT_BINARY_DIR}
  DEP_PROTO_TARGETS my_namespace::example_pb_gencode)
add_library(my_namespace::example_aimrt_rpc_gencode ALIAS example_aimrt_rpc_gencode)
```

&emsp;&emsp;这样之后，无论是Server端还是Client端，只要链接`my_namespace::example_aimrt_rpc_gencode`这个Target即可使用该协议与RPC方法。例如在作为Server端的`NormalRpcServerModule`模块下，在其CMakeLists.txt中使用如下代码即可：
```cmake
target_link_libraries(normal_rpc_server_module PUBLIC my_namespace::example_aimrt_rpc_gencode)
```


&emsp;&emsp;注意，无论在Servert端还是Client端，RPC的实现或调用目前都是以协程形式的接口提供的，使用者需要实现协程形式的RPC服务方法，并在协程中调用RPC。例如，在作为Server端的`NormalRpcServerModule`模块代码中，要实现具体的RPC逻辑，并注册RPC服务，示例代码如下：

- src/module/normal_rpc_server_module/rpc_service.h
  ```cpp
  // 继承桩代码中的service基类，实现相关接口
  class HardwareServiceImpl : public aimrt::protocols::example::ExampleService {
  public:
    HardwareServiceImpl() = default;
    ~HardwareServiceImpl() override = default;

    aimrt::co::Task<aimrt::rpc::Status> GetFooData(
        aimrt::rpc::ContextRef ctx,
        const ::aimrt::protocols::example::GetFooDataReq& req,
        ::aimrt::protocols::example::GetFooDataRsp& rsp) override;
  };
  ```
- src/module/normal_rpc_server_module/rpc_service.cc
  ```cpp
  // RPC处理函数，协程接口
  aimrt::co::Task<aimrt::rpc::Status> HardwareServiceImpl::GetFooData(
      aimrt::rpc::ContextRef ctx,
      const ::aimrt::protocols::example::GetFooDataReq& req,
      ::aimrt::protocols::example::GetFooDataRsp& rsp) {
    rsp.set_msg("echo " + req.msg());

    AIMRT_INFO("Server handle new rpc call. req: {}, return rsp: {}",
              aimrt::Pb2CompactJson(req), aimrt::Pb2CompactJson(rsp));

    co_return aimrt::rpc::Status();
  }
  ```
- src/module/normal_rpc_server_module/normal_rpc_server_module.cc
  ```cpp
  bool NormalRpcServerModule::Initialize(aimrt::CoreRef core) noexcept {
    core_ = core;

    // 注册RPC服务
    service_ptr_ = std::make_shared<HardwareServiceImpl>();
    core_.GetRpcHandle().RegisterService(service_ptr_);

    return true;
  }
  ```

&emsp;&emsp;在作为Client端的`NormalRpcClientModule`模块代码中，其示例调用代码如下：
```cpp
bool NormalRpcClientModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  // 注册RPC客户端方法
  aaimrt::rpc::RegisterClientFunc<aimrt::protocols::example::ExampleServiceProxy>(core_.GetRpcHandle());

  // 获取要跑任务的执行器句柄
  executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");

  return true;
}

bool NormalRpcClientModule::Start() noexcept {
  // 启动一个跑循环的协程
  scope_.spawn(MainLoop());
  return true;
}

void NormalRpcClientModule::Shutdown() noexcept {
  run_flag_ = false;
  co::SyncWait(scope_.complete());
}

// 跑循环的协程
aimrt::co::Task<void> NormalRpcClientModule::MainLoop() {
  try {
    auto proxy = std::make_shared<aimrt::protocols::example::ExampleServiceProxy>(core_.GetRpcHandle());

    aimrt::co::AimRTScheduler work_thread_pool_scheduler(executor_);

    while (run_flag_) {
      co_await aimrt::co::ScheduleAfter(
          work_thread_pool_scheduler, std::chrono::milliseconds(1000));

      // call rpc
      aimrt::protocols::example::GetFooDataReq req;
      aimrt::protocols::example::GetFooDataRsp rsp;
      req.set_msg("hello world");

      auto status = co_await proxy_->GetFooData(req, rsp);
    }

  } catch (const std::exception& e) {
    // ...
  }

  co_return;
}
```

&emsp;&emsp;需要注意的是，无论Server端还是Client端，都有个注册的步骤，要调用一个注册函数，Server端是`RegisterService`，客户端是`RegisterClientFunc`。这两个注册方法都必须在模块的`Initialize`方法内调用，不能在初始化之后再注册，否则会返回注册失败。


# 进阶使用

## AimRT目录结构介绍

&emsp;&emsp;`AimRT`的目录结构如下所示：
```
./src/
├── common/       # 一些基础库
├── examples/     # 示例
├── interface     # 接口层
│   ├── aimrt_core_plugin_interface/        # 插件开发接口
│   ├── aimrt_module_c_interface/           # 模块开发-C接口
│   ├── aimrt_module_cpp_interface/         # 模块开发-CPP接口
│   ├── aimrt_module_protobuf_interface/    # 模块开发-CPP-Protobuf接口
│   ├── aimrt_module_ros2_interface/        # 模块开发-CPP-Ros2接口
│   ├── aimrt_pkg_c_interface/              # Pkg接口
│   └── aimrt_runtime_c_interface/          # 框架运行时接口
├── plugins/    # 官方插件
│   ├── lcm_plugin/
│   ├── net_plugin/
│   ├── ros2_plugin/
│   └── sm_plugin/
├── protocols/  # 官方协议
├── runtime/
│   ├── core/     # 运行时核心类
│   ├── main/     # aimrt_main可执行文件
│   └── runtime/  # 运行时接口实现
└── tools/      # 工具
```

## aimrt_cli工具详细介绍
&emsp;&emsp;aimrt_cli工具详细介绍及使用指引详见: [aimrt_cli工具详解](./aimrt_cli_introduction.md)

## 协程

&emsp;&emsp;`AimRT`框架暴露给模块的所有原生接口都是纯C的，采用回调的形式处理一些异步逻辑。在这层C接口之上封装了一层CPP接口，提供了协程的形式来处理异步逻辑。



## 执行器实时性支持

&emsp;&emsp;`AimRT`框架的`thread`类型执行器支持实时性相关配置。请参考[example_real_time](https://code.agibot.com/agibot-tech/aimrt/-/tree/main/src/examples/example_real_time)示例。


## 使用插件

&emsp;&emsp;目前`AimRT`官方提供了几种插件：

1. [LCM插件](./LcmPlugin.md)
2. [共享内存插件](./SmPlugin.md)


## 插件开发

&emsp;&emsp;第三方开发者可以基于`AimRT`的插件接口开发自定义插件，从而实现一些自定义功能或增强框架能力。


## 通用协议

&emsp;&emsp;`AimRT`收集了一些机器人领域常用的协议，大部分是参照Ros2的协议形式。

## 接口层

&emsp;&emsp;`AimRT`在框架-模块、框架-插件之间都提供了接口层。其中在框架-模块之间提供了纯C接口，并在其之上封装了CPP接口层。除此之外`AimRT`还在纯C接口之上封装了`C#`接口，未来也会支持诸如`Python`、`Golang`、`Rust`等更多语言的接口。

## 测试

TODO

## 时间快进/慢放

TODO

# 示例

&emsp;&emsp;参考[examples](https://code.agibot.com/agibot-tech/aimrt/-/tree/main/src/examples)。

