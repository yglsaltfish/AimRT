
# HelloWorld CPP

[TOC]

&emsp;&emsp;本章将以一个简单的Demo来介绍如何建立一个最基本的AimRT CPP工程。

&emsp;&emsp;本Demo将演示以下几项基本功能：
- 基于CMake FetchContent通过源码引用AimRT；
- 编写一个基础的基于AimRT CPP接口的`Module`；
- 使用基础的日志功能；
- 使用基础的基础配置功能；
- 以App模式集成`Module`；
- 编译项目，并运行进程以执行`Module`中的逻辑。


&emsp;&emsp;更多示例，请参考AimRT代码仓库中的[examples](https://code.agibot.com/agibot_aima/aimrt/-/tree/main/src/examples/cpp)。

## STEP1: 确保本地环境符合要求

&emsp;&emsp;请先确保本地的编译环境、网络环境符合要求，具体请参考[引用与安装](installation.md)中的要求。

&emsp;&emsp;注意，本示例是跨平台的，但本章节基于linux进行演示。


## STEP2: 创建目录结构，并添加基本的文件

&emsp;&emsp;参照以下目录结构创建文件：
```
├── CMakeLists.txt
├── cmake
│   └── GetAimRT.cmake
└── src
    ├── CMakeLists.txt
    ├── install
    │   └── cfg
    │       └── helloworld_cfg.yaml
    ├── module
    │   └── helloworld_module
    │       ├── CMakeLists.txt
    │       ├── helloworld_module.cc
    │       └── helloworld_module.h
    └── app
        └── helloworld_app
            ├── CMakeLists.txt
            └── main.cc
```

&emsp;&emsp;请注意，此处仅是一个供参考的路径结构，并非强制要求。但推荐您在搭建自己的工程时，为以下几个领域单独建立文件夹：
- install：存放部署时的一些配置、启动脚本等
- module：存放业务逻辑代码
- app：app模式下，main函数所在地，在main函数中注册业务module
- pkg：pkg模式下，动态库入口方法所在地，在pkg中注册业务module


&emsp;&emsp;其中，一些关键的CMake文件内容如下：

### File 1 : /CMakeLists.txt
&emsp;&emsp;根CMake，用于构建工程。
```cmake
cmake_minimum_required(VERSION 3.25)

project(helloworld LANGUAGES C CXX)

# 需要启用C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 获取aimrt
include(cmake/GetAimRT.cmake)

# 引用下层cmake
add_subdirectory(src)
```

### File 2 : /cmake/GetAimRT.cmake
&emsp;&emsp;此文件用于获取AimRT。
```cmake
include(FetchContent)

# 请在此处选择使用的版本
FetchContent_Declare(
  aimrt #
  GIT_REPOSITORY http://code.agibot.com/agibot_aima/aimrt.git #
  GIT_TAG v0.6.0)

FetchContent_GetProperties(aimrt)

if(NOT aimrt_POPULATED)
  FetchContent_MakeAvailable(aimrt)
endif()
```

### File 3 : /src/CMakeLists.txt
&emsp;&emsp;描述src下的各个cmake target。
```cmake
add_subdirectory(module/helloworld_module)
add_subdirectory(app/helloworld_app)
```

### File 4 : /src/module/helloworld_module/CMakeLists.txt
&emsp;&emsp;描述helloworld_module。
```cmake
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(helloworld_module STATIC)
add_library(helloworld::helloworld_module ALIAS helloworld_module)

target_sources(helloworld_module PRIVATE ${src})

target_include_directories(
  helloworld_module
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# 引用aimrt_module_cpp_interface
target_link_libraries(
  helloworld_module
  PRIVATE yaml-cpp::yaml-cpp
  PUBLIC aimrt::interface::aimrt_module_cpp_interface)
```

### File 5 : /src/app/helloworld_app/CMakeLists.txt
&emsp;&emsp;描述helloworld_app。
```cmake
file(GLOB_RECURSE src ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_executable(helloworld_app)

target_sources(helloworld_app PRIVATE ${src})

target_include_directories(
  helloworld_app
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  helloworld_app
  PRIVATE aimrt::runtime::core
          helloworld::helloworld_module)
```

## STEP3: 开发业务逻辑

&emsp;&emsp;业务逻辑主要通过`Module`来承载，参考以下代码实现一个简单的`Module`，解析传入的配置文件并打印一些简单的日志：

### File 6 : src/module/helloworld_module/helloworld_module.h
```cpp
#pragma once

#include "aimrt_module_cpp_interface/module_base.h"

class HelloWorldModule : public aimrt::ModuleBase {
public:
  HelloWorldModule() = default;
  ~HelloWorldModule() override = default;

  aimrt::ModuleInfo Info() const override {
    return aimrt::ModuleInfo{.name = "HelloWorldModule"};
  }

  bool Initialize(aimrt::CoreRef core) override;
  bool Start() override;
  void Shutdown() override;

private:
  aimrt::CoreRef core_;
};
```

### File 7 : src/module/helloworld_module/helloworld_module.cc
```cpp
#include "helloworld_module/helloworld_module.h"

#include "yaml-cpp/yaml.h"

bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  // Save aimrt framework handle
  core_ = core;

  // Log
  AIMRT_HL_INFO(core_.GetLogger(), "Init.");

  try {
    // Read cfg
    auto configurator = core_.GetConfigurator();
    if (configurator) {
      YAML::Node cfg_node = YAML::LoadFile(std::string(configurator.GetConfigFilePath()));
      for (const auto &itr : cfg_node) {
        std::string k = itr.first.as<std::string>();
        std::string v = itr.second.as<std::string>();
        AIMRT_HL_INFO(core_.GetLogger(), "cfg [{} : {}]", k, v);
      }
    }

  } catch (const std::exception& e) {
    AIMRT_HL_ERROR(core_.GetLogger(), "Init failed, {}", e.what());
    return false;
  }

  AIMRT_HL_INFO(core_.GetLogger(), "Init succeeded.");

  return true;
}

bool HelloWorldModule::Start() {
  AIMRT_HL_INFO(core_.GetLogger(), "Start succeeded.");
  return true;
}

void HelloWorldModule::Shutdown() {
  AIMRT_HL_INFO(core_.GetLogger(), "Shutdown succeeded.");
}
```

## STEP4: 制定部署方案

&emsp;&emsp;我们使用App模式，手动编写Main函数，将HelloWorldModule通过硬编码的方式注册到AimRT框架中。然后编写一份配置，以确定日志等细节。参考以下代码：

### File 8 : /src/app/helloworld_app/main.cc
&emsp;&emsp;在以下示例main函数中，我们捕获了kill信号，以完成优雅退出。
```cpp
#include <csignal>
#include <iostream>

#include "core/aimrt_core.h"
#include "helloworld_module/helloworld_module.h"

using namespace aimrt::runtime::core;

AimRTCore *global_core_ptr_ = nullptr;

void SignalHandler(int sig) {
  if (global_core_ptr_ && (sig == SIGINT || sig == SIGTERM)) {
    global_core_ptr_->Shutdown();
    return;
  }
  raise(sig);
};

int32_t main(int32_t argc, char **argv) {

  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);

  std::cout << "AimRT start." << std::endl;

  try {
    AimRTCore core;
    global_core_ptr_ = &core;

    // register module
    HelloWorldModule helloworld_module;
    core.GetModuleManager().RegisterModule(helloworld_module.NativeHandle());

    AimRTCore::Options options;
    options.cfg_file_path = argv[1];
    core.Initialize(options);

    core.Start();

    core.Shutdown();

    global_core_ptr_ = nullptr;
  } catch (const std::exception &e) {
    std::cout << "AimRT run with exception and exit. " << e.what() << std::endl;
    return -1;
  }

  std::cout << "AimRT exit." << std::endl;
  return 0;
}
```

### File 9 : /src/install/cfg/helloworld_cfg.yaml
&emsp;&emsp;以下是一个简单的示例配置文件。这个配置文件中的其他内容将在后续章节中介绍，这里关注两个地方：
- `aimrt.log`节点：此处指定了日志的一些细节。
- `HelloWorldModule`节点：此处为`HelloWorldModule`的配置，可以在模块中读取到。

```yaml
aimrt:
  configurator:
    temp_cfg_path: ./cfg/tmp # 生成的临时模块配置文件存放路径
  log: # log配置
    core_lvl: INFO # 内核日志等级，可选项：Trace/Debug/Info/Warn/Error/Fatal/Off，不区分大小写
    default_module_lvl: INFO # 模块默认日志等级
    backends: # 日志backends
      - type: console # 控制台日志
        options:
          color: true # 是否彩色打印
      - type: rotate_file # 文件日志
        options:
          path: ./log # 日志文件路径
          filename: helloworld.log # 日志文件名称
          max_file_size_m: 4 # 日志文件最大尺寸，单位m
          max_file_num: 10 # 最大日志文件数量，0代表无限
  module: # 模块配置
    modules: # 模块
      - name: HelloWorldModule # 模块Name接口返回的名称
        log_lvl: INFO # 模块日志级别

# 模块自定义配置，框架会为每个模块生成临时配置文件，开发者通过Configurator接口获取该配置文件路径
HelloWorldModule:
  key1: val1
  key2: val2

```

## STEP5: 启动并测试

&emsp;&emsp;完善代码之后，在linux上执行以下命令完成编译：
```shell
# 先cd到代码根目录
cmake -B build
cd build
make -j
```

&emsp;&emsp;编译完成后，将生成的可执行文件`helloworld_app`和配置文件`helloworld_cfg.yaml`拷贝到一个目录下，然后执行以下命令运行进程，观察打印出来的日志：
```shell
./helloworld_app ./helloworld_cfg.yaml
```

&emsp;&emsp;如果是在其他平台上，例如windows上，也可以参考linux上的步骤进行编译、运行。
