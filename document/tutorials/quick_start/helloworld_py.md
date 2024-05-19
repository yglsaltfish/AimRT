
# HelloWorld Python

[TOC]


***TODO待完善***

&emsp;&emsp;本章将以一个简单的Demo来介绍如何建立一个最基本的AimRT Python工程。


&emsp;&emsp;aimrt_py是基于pybind11，在AimRT CPP接口层之上包装的一层python接口，其接口风格与CPP接口非常类似。本Demo将演示以下几项基本功能：
- 编写一个基础的基于AimRT Python接口的`Module`；
- 使用基础的日志功能；
- 使用基础的基础配置功能；
- 在Python代码中集成`Module`；
- 运行Python工程以执行`Module`中的逻辑。


&emsp;&emsp;更多示例，请参考AimRT代码仓库中的[examples](https://code.agibot.com/agibot_aima/aimrt/-/tree/main/src/examples/py)。


## STEP1：获取aimrt_py组件

&emsp;&emsp;当前以二进制的形式分发，下载地址：[aimrt_py](https://code.agibot.com/wangtian/aimrt-py)。


## STEP2：由protobuf协议文件生成py桩代码文件

&emsp;&emsp;如果是仅使用channel功能，则只需要使用protoc工具生成`xxx_pb2.py`文件即可，例如执行以下命令：
```
./protoc -I/path/to/proto/dir --python_out=./ /path/to/proto/dir/xxx.proto
```
&emsp;&emsp;此命令将根据`xxx.proto`文件生成`xxx_pb2.py`文件，业务需要引用生成的该python文件。


&emsp;&emsp;如果要使用rpc功能，则还需要为包含service语句的proto文件生成`xxx_aimrt_rpc_pb2.py`文件，例如执行以下命令：
```
./protoc -I/path/to/proto/dir --aimrt_rpc_out=./ --plugin=protoc-gen-aimrt_rpc=/path/to/tool/protoc_plugin_py_gen_aimrt_rpc.py /path/to/proto/dir/xxx.proto
```
&emsp;&emsp;此命令将根据`xxx.proto`文件中的service语句生成`xxx_aimrt_rpc_pb2.py`文件，业务需要引用生成的该python文件。


## STEP3：编写业务代码

### 业务模块代码
&emsp;&emsp;参考example，编写业务模块类。业务模块类需要继承`aimrt_py.ModuleBase`，实现以下几个接口：
- `Info(self) -> aimrt_py.ModuleInfo`：返回模块信息，最主要的是模块名称
- `Initialize(self, core) -> bool`：模块初始化方法。框架在初始化时调用该接口
- `Start(self) -> bool`：模块启动分发。框架在启动时调用该接口
- `Shutdown(self) -> void`：模块停止接口。框架在停止时调用该接口

&emsp;&emsp;大部分框架功能在Start后可用，在Initialize阶段不要做业务逻辑。

### App代码
&emsp;&emsp;编写完业务模块后，在main函数中创建一个`aimrt_py.Core`实例，并将模块注册进去。然后参考示例，将配置文件路径等启动配置也传递给`aimrt_py.Core`实例，调用Start方法。


### 配置文件
&emsp;&emsp;参考示例，编写配置文件。配置文件是C++/python通用的，一般用于配置日志、执行器、模块配置、rpc/channel底层通信方式等。


## step4：启动app
&emsp;&emsp;参考示例，直接启动Python编写的App脚本，带上配置文件地址作为参数。


