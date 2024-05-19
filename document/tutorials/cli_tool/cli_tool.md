
# CLI工具

[TOC]


## 简介

&emsp;&emsp;**aimrt_cli**是一个AimRT官方提供的命令行工具，目前支持以下功能：

- [为新项目生成脚手架代码](./gen_prj.md)

&emsp;&emsp;您也可以直接执行`aimrt_cli --help`获取内置帮助说明。更多功能敬请期待。


## 安装
&emsp;&emsp;**aimrt_cli**工具是一个基于Python开发的小程序，有以下三种安装方式，请选择任意一种您喜欢的进行安装。


### 源码安装到python环境中
&emsp;&emsp;**aimrt_cli**提供了通过`setuptools`工具直接打包到python环境中的功能，您可下载AimRT源码后，在终端中执行以下命令:
```
cd <path_to_your_aimrt_src_code>/src/tools/aimrt_cli
python setup.py install
```
&emsp;&emsp;**aimrt_cli**工具将会自动安装到您的python环境中。使用 `pip list | grep aimrt_cli`可查看是否安装成功。可使用 `pip uninstall aimrt_cli`进行卸载。


### 源码编译出可执行文件
&emsp;&emsp;可直接通过编译**aimrt**库编译出 `aimrt_cli`的可执行文件，并将其加入到系统的环境变量中，即可在终端中执行 `aimrt_cli`命令。整体流程为:
- 执行您aimrt源码库中的`build.sh`文件进行编译。可以在`build.sh`文件中关闭其他您不需要的CMake选项以加快编译。
- 在`build`文件夹下可找到编译出的**aimrt_cli**可执行文件。
+ 将其添加到环境变量中。

&emsp;&emsp;参考以下命令：
```
cd <path_to_your_aimrt_src_code>
./build.sh

mv <path_to_your_aimrt_src_code>/build/install/bin/aimrt_cli <path_to_your_aimrt_cli>

export PATH=<path_to_your_aimrt_cli>:$PATH
```


### 从Release中下载可执行文件

***TODO建设中***

&emsp;&emsp;您可在[Release下载地址]()中找到所需的发布版本，将其下载后解压，并将其加入到系统的环境变量中，即可在终端中执行 `aimrt_cli`命令。参考以下命令：
```
export PATH=<path_to_your_aimrt_cli>:$PATH
```
