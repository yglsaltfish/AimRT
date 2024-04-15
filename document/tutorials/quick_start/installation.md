
# 安装

&emsp;&emsp;AimRT提供了两种方式进行安装：
- 从源码编译安装【推荐】
- 二进制安装

&emsp;&emsp;AimRT比较轻量，推荐用户直接使用源码进行编译安装。甚至用户可以直接使用CMake FetchContent，通过源码进行引用。


## 从源码构建安装

您可以直接下载AimRT代码到本地，然后进行构建安装，最后在本地项目的CMake里使用find_package找到AimRT。

### Step1：确保本地环境符合要求

&emsp;&emsp;AimRT兼容linux、windows等主流操作系统，编译器需要能够支持c++20，使用CMake 3.25或以上版本构建。我们已经在以下操作系统和编译器上测试过：
- Ubuntu20.04
  - gcc-11.4
  - gcc-13.2
  - clang-16.0.6
- Ubuntu22.04
  - gcc-11.4
  - gcc-13.2
  - clang-16.0.6
- Windows11
  - MSVC-19.36


### Step2：编译安装
&emsp;&emsp;首先通过git等方式下载源码，然后基于CMake进行构建编译，在构建时传入`CMAKE_INSTALL_PREFIX`选项指定安装地址，构建完成后执行install安装。

&emsp;&emsp;请注意：
- 在编译构建时，AimRT会通过源码方式引用一些第三方依赖，如果出现问题，可以参考[CMake](../concepts/cmake.md)文档进行处理。
- 如果要编译ROS2相关接口/插件，AimRT会通过find_package的方式在本地寻找rclcpp等依赖，请确保本地安装有[ROS2 Humble](https://docs.ros.org/en/humble/)。
- 如果要构建Python接口、cli工具等，AimRT会通过find_package的方式在本地寻找Python依赖，请确保本地安装有Python3。


## 从二进制包安装
&emsp;&emsp;您可以直接在[AimRT的发布页面]()上下载一些主流平台上编译好的二进制包并安装，之后即可在cmake中使用find_package等方法引入到您的工程中。

&emsp;&emsp;注意：
- 部分插件只在一些平台上提供，这和插件本身所需的组件支持的平台有关。
- AimRT二进制安装包形式直接支持的平台较少，但并不意味AimRT仅支持这些平台。AimRT本身比较轻量，没有太多依赖，鼓励使用源码形式安装/引用。



