
# 引用与安装

[TOC]


&emsp;&emsp;您可以通过两种方式引用AimRT：
- 基于CMake FetchContent，通过源码进行引用【推荐】
- 安装后，基于CMake find_package进行引用

AimRT比较轻量，推荐用户直接基于源码进行引用。如果要使用基于安装的方式进行引用，AimRT也提供了两种方式进行安装：
- 从源码编译安装
- 二进制安装


## 编译环境要求
&emsp;&emsp;AimRT兼容linux、windows等主流操作系统，编译器需要能够支持c++20，使用CMake 3.25或以上版本构建。我们已经在以下操作系统和编译器上测试过：
- Ubuntu22.04
  - gcc-11.4
  - gcc-12.3
  - gcc-13.2
  - clang-16.0.6
  - clang-17.0.6
- Windows11
  - MSVC-19.36

&emsp;&emsp;请注意：
- 在编译构建时，AimRT可能通过源码方式引用一些第三方依赖，如果出现网络问题，可以参考[CMake](../concepts/cmake.md)文档进行处理。
- 如果要编译ROS2相关接口/插件，AimRT会通过find_package的方式在本地寻找rclcpp等依赖，请确保本地安装有[ROS2 Humble](https://docs.ros.org/en/humble/)。
- 如果要构建Python接口、cli工具等，AimRT会通过find_package的方式在本地寻找Python依赖，请确保本地安装有Python3。


## 基于CMake FetchContent，通过源码进行引用

&emsp;&emsp;您可以参考以下CMake代码引用AimRT：
```cmake
include(FetchContent)

# 可以指定aimrt地址和版本
FetchContent_Declare(
  aimrt
  GIT_REPOSITORY http://code.agibot.com/agibot_aima/aimrt.git
  GIT_TAG v1.x.x)

FetchContent_GetProperties(aimrt)

if(NOT aimrt_POPULATED)
  # 设置AimRT的一些编译选项
  set(AIMRT_BUILD_TESTS OFF CACHE BOOL "")
  set(AIMRT_BUILD_EXAMPLES OFF CACHE BOOL "")

  FetchContent_MakeAvailable(aimrt)
endif()

# 引入后直接使用target_link_libraries链接aimrt的target
target_link_libraries(
  my_module
  PUBLIC aimrt::interface::aimrt_module_cpp_interface)
```

## 安装后，基于CMake find_package进行引用

***TODO，此方式还待完善，暂不建议使用***

### 安装方式一：从源码构建安装

&emsp;&emsp;首先通过git等方式下载源码，然后基于CMake进行构建编译，在构建时传入`CMAKE_INSTALL_PREFIX`选项指定安装地址，构建完成后执行install安装。

### 安装方式二：从二进制包安装

&emsp;&emsp;您可以直接在[AimRT的发布页面]()上下载一些主流平台上编译好的二进制包并安装。

&emsp;&emsp;注意：
- 部分插件只在一些平台上提供，这和插件本身所需的组件支持的平台有关。
- AimRT二进制安装包形式直接支持的平台较少，但并不意味AimRT仅支持这些平台。AimRT本身比较轻量，没有太多依赖，鼓励使用源码形式安装/引用。


### 安装完成后，使用CMake find_package进行引用

