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



### step3：编写业务代码






