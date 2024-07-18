# 引用与安装（Python）

AimRT Python 接口通过 `aimrt_py` 包来使用。您可以通过三种方式安装获取 `aimrt_py` 包：

- 基于 pip install 安装；
- 二进制安装；
- 基于源码编译安装；

## Python 环境要求

最低要求的 Python 版本是 3.10，Linux 系统 glibc 的版本最低为 2.28（可以使用 `ldd --version` 命令查看）。

我们在以下系统和 python 版本上测试过 `aimrt_py` 包：

- Ubuntu22.04
  - python3.10
- Windows11
  - python3.11

<!-- - Ubuntu22.04
  - python3.10
  - python3.11
  - python3.12
- Windows11
  - python3.10
  - python3.11
  - python3.12 -->

请注意，如果您想要使用 AimRT-Python 中的 RPC 或 Channel 功能，当前只支持以 protobuf 作为协议，在使用时需要在本地安装有 protobuf python 包，您可以通过 `pip install protobuf` 来安装。

## 基于 pip install 安装

***TODO，此方式还在建设中***

您可以直接通过 `pip install aimrt_py` 来安装。

## 二进制安装

***TODO，此方式还在建设中***

您可以直接在 [AimRT 的发布页面](https://code.agibot.com/agibot_aima/aimrt) 上下载一些主流平台上编译好的二进制包，并在其中找到 aimrt_py 的 whl 文件，通过 pip 安装。

当前下载地址：[aimrt_py](https://file.agibot.com/aimrt_py/e7680395c305ac0333b6f2dabcd03c8269544d03)，目前仅含 Ubuntu 22.04 平台上编译的 x86_64 和 aarch64 版本。

## 基于源码编译安装

首先通过 git 等方式下载源码，然后基于 CMake 进行构建编译，构建完成后在 build/aimrt_py_package/dist 路径下有 aimrt_py 的 whl 文件，最后通过 pip 安装。

以下为逐步详细指导：

1. 下载源码

   可以使用 git 下载，使用如下命令

   ```bash
   git clone http://code.agibot.com/agibot_aima/aimrt.git
   ```

   也可以直接在 [仓库主页](https://code.agibot.com/agibot_aima/aimrt) 下载 .zip 包进行然后解压，效果相同。

2. 安装编译依赖

   全量编译本项目需要安装 ros2（humble 版本，安装指引见 [ros2 installation](https://docs.ros.org/en/humble/Installation.html)），如果您不需要使用 ros2 插件可以在 build.sh 文件中将以下选项更改为 OFF。

   ```bash
   -DAIMRT_BUILD_WITH_ROS2=ON \
   -DAIMRT_BUILD_ROS2_PLUGIN=ON \
   ```

   应该修改为

    ```bash
    -DAIMRT_BUILD_WITH_ROS2=OFF \
    -DAIMRT_BUILD_ROS2_PLUGIN=OFF \
    ```

   为支持打包为 .whl 文件，需要安装 setuptools 和 build 两个 python 模块以及 python-venv ，可以使用如下命令安装

    ```bash
    pip install setuptools build
    sudo apt install python-venv
    ```

    为编译项目还需要安装 CMake，可以使用如下命令安装

    ```bash
    sudo apt install cmake
    ```

    CMake 的最低版本要求为 3.22，如果系统中的版本低于 3.12，可以到 [CMake 官网](https://cmake.org/download/) 下载最新版本的 CMake 并安装。

3. 执行 build.sh 脚本编译生成 .whl 文件

    进入 aimrt 项目文件夹，执行 build.sh 脚本，命令如下

    ```bash
    cd aimrt
    ./build.sh
    ```

    编译完成后可以在 `build/aimrt_py_pkg/dist` 目录下找到 `aimrt_py*.whl` 文件，使用不同的 python 环境、系统架构和仓库版本文件名会略有不同，例如使用 CPython3.10 在 x86_64 架构下编译 0.7.0 版本仓库生成的文件名为 `aimrt_py-0.7.0-cp310-cp310-linux_x86_64.whl`。

4. 使用 pip 安装 .whl 文件

    使用 pip 安装编译生成的 .whl 文件，命令如下

    ```bash
    pip install build/aimrt_py_pkg/dist/aimrt_py*.whl
    ```

    安装完成后即可使用 aimrt_py 包。
