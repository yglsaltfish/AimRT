
# Installation

本章将介绍如何安装AimRT。

（TODO：待完善）

## 系统环境要求
AimRT兼容linux、windows等主流操作系统。我们已经在以下操作系统上测试过：
- Ubuntu22.04
- Windows11


编译器需要能够支持c++20。我们已经在以下编译器版本上测试过：
- gcc-11.3
- clang-16.0.2
- MSVC-19.36

 
## 源码构建（推荐）
AimRT非常轻量，推荐直接从源码引用。

AimRT使用CMake构建，推荐使用CMake FetchContent进行引用。详细的源码引用方式见[]()。

## 二进制包安装

您可以直接在AimRT的发布页面上下载一些平台上编译好的二进制包。


## 源码安装

您可以直接通过git clone或下载源码包的方式将源码下载到本地，然后使用CMake进行构建安装。

如果是在linux平台上，可以直接运行`build.sh`进行构建。


