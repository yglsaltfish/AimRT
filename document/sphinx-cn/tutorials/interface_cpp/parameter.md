# Parameter

## ParameterHandleRef句柄概述

相关链接：
- 代码文件：[aimrt_module_cpp_interface/parameter/parameter_handle.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/parameter/parameter_handle.h)
- 参考示例：[parameter_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/parameter/module/parameter_module/parameter_module.cc)


AimRT中提供了一个简单的模块级kv参数功能，模块可以通过调用`CoreRef`句柄的`GetParameterHandle()`接口，获取`aimrt::parameter::ParameterHandleRef`句柄，来使用此功能。该句柄提供的核心接口如下：

```cpp
namespace aimrt::parameter {

class ParameterHandleRef {
 public:
  std::string GetParameter(std::string_view key) const;

  void SetParameter(std::string_view key, std::string_view val) const;
};

}  // namespace aimrt::parameter
```

使用注意点如下：
- `std::string GetParameter(std::string_view key)`接口：用于获取参数。
  - 如果不存在key，则返回空字符串。
  - 该接口是线程安全的。
- `void SetParameter(std::string_view key, std::string_view val)`接口：用于设置/更新参数。
  - 如果不存在key，则新建一个key-val参数对。
  - 如果存在key，则更新key所对应的val值为最新值。
  - 该接口是线程安全的。
- 无论是设置参数还是获取参数，都是模块级别的，不同模块的参数互相独立、互不可见。


一个简单的使用示例如下：
```cpp
bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  auto parameter_handle = core_.GetParameterHandle();

  std::string key = "test key";
  std::string val = "test val";

  // Set
  parameter_handle_.SetParameter(key, val);

  // Get
  std::string check_val = parameter_handle_.GetParameter(key);

  return true;
}
```

除了通过本小节所介绍的CPP模块接口中的参数接口来设置/获取参数，使用者也可以通过parameter_plugin，通过RPC或者HTTP等方式来设置/获取参数。具体请参考[parameter_plugin的文档](../plugins/parameter_plugin.md)。

