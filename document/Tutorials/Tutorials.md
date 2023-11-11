
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



&emsp;&emsp;其中，在初始化时，框架会给模块传入一个`aimrt::CoreRef`结构。


### 使用配置功能

&emsp;&emsp;在初始化时，框架会给模块传入一个`aimrt::CoreRef`结构，使用其`GetConfigurator()`接口返回的`ConfiguratorBase`指针，可以通过其`GetConfigFilePath()`方法获取模块配置文件的路径，模块可以依据此在初始化方法中自行读取配置文件：
```cpp
bool HelloWorldModule::Initialize(agiros::AgiRosBase* agiros_ptr) noexcept {
  agiros_ptr_ = agiros_ptr;

  try {
    const agiros::ConfiguratorBase* configurator = agiros_ptr_->GetConfigurator();
    if (configurator != nullptr) {
      // 根据返回的配置文件路径，使用相应方式读取模块配置文件。例如这里使用yaml-cpp打开该配置文件
      YAML::Node cfg_node = YAML::LoadFile(configurator->GetConfigFilePath());

      // ...
    }
  }

  return true;
}
```

&emsp;&emsp;注意：如果在框架根配置yaml文件的模块配置中指定了模块配置文件路径，则此处返回指定的文件路径。例如：
```yaml
agiros:
  module: # 模块配置
    modules: # 模块
      - name: HelloWorldModule # 模块Name接口返回的名称
        cfg_file_path: ./cfg/my_module_cfg.txt
```
&emsp;&emsp;则此时`GetConfigFilePath()`方法返回值为`./cfg/my_module_cfg.txt`。该配置文件为用户自定义，不限制格式。

&emsp;&emsp;如果在框架根配置yaml文件中存在以模块名为名称的节点，则框架会为该模块生成一个临时yaml配置文件，并将模块配置写入到此临时文件中，此时`GetConfigFilePath()`方法返回值为这个临时配置文件的路径。例如框架的根配置yaml文件如下所示时：
```yaml
agiros:
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
&emsp;&emsp;则此时`GetConfigFilePath()`方法将返回一个临时配置文件路径，该临时配置文件将位于`agiros.configurator.temp_cfg_path`节点所配置的目录下，其中的内容如下：
```yaml
key1: val1
key2: val2
```


### 使用日志功能



### 使用执行器



### 使用channel通信


### 使用rpc通信



# 进阶使用

## aimrt_cli工具详细介绍
aimrt_cli工具详细介绍及使用指引详见: [aimrt_cli工具详解](./aimrt_cli_introduction.md)

## 协程

## 执行器实时性支持


## 使用插件


## 官方插件详细介绍

1. [LCM插件](./LcmPlugin.md)

2. [共享内存插件](./SmPlugin.md)


## 插件开发


## 通用协议


## 接口层


## 测试


## 时间快进/慢放



# 示例


