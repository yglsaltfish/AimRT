# CoreRef

## CoreRef句柄概述

相关链接：
- 代码文件：[aimrt_module_cpp_interface/core.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/core.h)
- 参考示例：[helloworld_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/helloworld/module/helloworld_module/helloworld_module.cc)


`aimrt::CoreRef`是调用框架功能的句柄类型，可以通过以下两种方式获取：
- 继承`ModuleBase`类型实现自己的`Module`，在`Module`的`Initialize`方法中，AimRT框架会传入一个`aimrt::CoreRef`句柄；
- App模式下，通过Create Module方式，AimRT运行时会在创建一个`Module`后返回一个`aimrt::CoreRef`句柄；

`aimrt::CoreRef`中提供的核心接口如下：

```cpp
namespace aimrt {

class CoreRef {
 public:
  ModuleInfo Info() const;

  configurator::ConfiguratorRef GetConfigurator() const;

  allocator::AllocatorRef GetAllocator() const;

  executor::ExecutorManagerRef GetExecutorManager() const;

  logger::LoggerRef GetLogger() const;

  rpc::RpcHandleRef GetRpcHandle() const;

  channel::ChannelHandleRef GetChannelHandle() const;

  parameter::ParameterHandleRef GetParameterHandle() const;
};

}  // namespace aimrt
```


`aimrt::CoreRef`的使用注意点：
- AimRT框架会为每个模块生成一个专属`CoreRef`句柄，以实现资源隔离、监控等方面的功能。可以通过`CoreRef::Info`接口获取其所属的模块的信息。
- 可以通过`CoreRef`中的`GetXXX`接口获取对应组件的句柄，来调用相关功能。

一个简单的使用示例如下：
```cpp
bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  // Get log handle
  auto logger = core.GetLogger();

  // Use log handle
  AIMRT_HL_INFO(logger, "This is a test log");

  return true;
}
```

