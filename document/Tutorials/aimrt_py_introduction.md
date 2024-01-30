# aimrt_py 使用指南


## 1. 简介

aimrt_py是基于pybind11，在AimRT CPP接口层之上包装的一层python接口。其接口风格与CPP接口非常类似，具体可以参考以下几个示例：
- [example_helloworld_py]()：示例log、执行器、定时器、配置等功能
- [example_normal_channel_py]()：示例channel功能（基于protobuf协议类型）
- [example_normal_rpc_py]()：示例rpc功能（基于protobuf协议类型）


请注意：当前（2024.01.18）只支持protobuf协议类型，ros2协议类型将在未来支持。


## 2. 使用步骤

### step1：获取aimrt_py组件

当前以二进制的形式分发，下载地址（需连接智元内网）：[aimrt_py](https://file.agibot.com/aimrt_py)。


### step2：由protobuf协议文件生成py桩代码文件

如果是仅使用channel功能，则只需要使用protoc工具生成`xxx_pb2.py`文件即可，例如执行以下命令：
```
./protoc -I/path/to/proto/dir --python_out=./ /path/to/proto/dir/xxx.proto
```
此命令将根据`xxx.proto`文件生成`xxx_pb2.py`文件，业务需要引用生成的该python文件。


如果要使用rpc功能，则还需要为包含service语句的proto文件生成`xxx_aimrt_rpc_pb2.py`文件，例如执行以下命令：
```
./protoc -I/path/to/proto/dir --aimrt_rpc_out=./ --plugin=protoc-gen-aimrt_rpc=/path/to/tool/protoc_plugin_py_gen_aimrt_rpc.py /path/to/proto/dir/xxx.proto
```
此命令将根据`xxx.proto`文件中的service语句生成`xxx_aimrt_rpc_pb2.py`文件，业务需要引用生成的该python文件。


### step3：编写业务代码

#### 业务模块代码
参考example，编写业务模块类。业务模块类需要继承`aimrt_py.ModuleBase`，实现以下几个接口：
- `Info(self) -> aimrt_py.ModuleInfo`：返回模块信息，最主要的是模块名称
- `Initialize(self, core) -> bool`：模块初始化方法。框架在初始化时调用该接口
- `Start(self) -> bool`：模块启动分发。框架在启动时调用该接口
- `Shutdown(self) -> void`：模块停止接口。框架在停止时调用该接口

大部分框架功能在Start后可用，在Initialize阶段不要做业务逻辑。

#### App代码
编写完业务模块后，在main函数中创建一个`aimrt_py.Core`实例，并将模块注册进去。然后参考示例，将配置文件路径等启动配置也传递给`aimrt_py.Core`实例，调用Start方法。


#### 配置文件
参考示例，编写配置文件。配置文件是C++/python通用的，一般用于配置日志、执行器、模块配置、rpc/channel底层通信方式等。


### step4：启动app
参考示例，直接启动Python编写的App脚本，带上配置文件地址作为参数。


