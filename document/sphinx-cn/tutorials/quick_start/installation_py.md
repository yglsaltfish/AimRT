
# 引用与安装（Python）

AimRT Python接口通过`aimrt_py`包的来使用。您可以通过三种方式安装获取`aimrt_py`包：
- 基于pip install安装；
- 二进制安装；
- 基于源码编译安装；

## Python环境要求

我们在以下系统和python版本上测试过`aimrt_py`包：
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

请注意，如果你想要使用AimRT-Python中的RPC或Channel功能，当前只支持以protobuf作为协议，在使用时需要在本地安装有protobuf python包，你可以通过`pip install protobuf`来安装。


## 基于pip install安装

***TODO，此方式还在建设中***

你可以直接通过`pip install aimrt_py`来安装。


## 二进制安装

***TODO，此方式还在建设中***

您可以直接在[AimRT的发布页面](https://code.agibot.com/agibot_aima/aimrt)上下载一些主流平台上编译好的二进制包，并在其中找到aimrt_py的whl文件，通过pip安装。

当前下载地址：[aimrt_py](https://code.agibot.com/wangtian/aimrt-py)。


## 基于源码编译安装

***TODO，此方式还在建设中***

首先通过git等方式下载源码，然后基于CMake进行构建编译，构建完成后在build路径下有aimrt_py的whl文件，然后通过pip安装。

