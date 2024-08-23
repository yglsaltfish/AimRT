# ModuleBase

## ModuleBase类型概述

相关链接：
- 代码文件：{{ '[aimrt_module_cpp_interface/module_base.h]({}/src/interface/aimrt_module_cpp_interface/module_base.h)'.format(code_site_root_path_url) }}
- 参考示例：{{ '[helloworld_module.cc]({}/src/examples/cpp/helloworld/module/helloworld_module/helloworld_module.cc)'.format(code_site_root_path_url) }}


`ModuleBase`类型是一个模块基类类型，开发者可以继承`ModuleBase`类型来实现自己的`Module`，它定义了业务模块所需要实现的几个接口：

```cpp
namespace aimrt {

class ModuleBase {
 public:
  virtual ModuleInfo Info() const = 0;

  virtual bool Initialize(CoreRef core) = 0;

  virtual bool Start() = 0;

  virtual void Shutdown() = 0;
};

}  // namespace aimrt
```

其中`aimrt::ModuleInfo`结构体声明如下：
```cpp
namespace aimrt {

struct ModuleInfo {
  std::string_view name;  // Require

  uint32_t major_version = 0;
  uint32_t minor_version = 0;
  uint32_t patch_version = 0;
  uint32_t build_version = 0;

  std::string_view author;
  std::string_view description;
};

}  // namespace aimrt
```

关于这些虚接口，说明如下：
- `ModuleInfo Info()`：用于AimRT框架获取模块信息，包括模块名称、模块版本等。
  - AimRT框架会在加载模块时调用此接口，读取模块信息。
  - `ModuleInfo`结构中除`name`是必须项，其余都是可选项。
  - 如果模块在其中抛了异常，等效于返回一个空ModuleInfo。
- `bool Initialize(CoreRef)`：用于初始化模块。
  - AimRT框架在Initialize阶段，依次调用各模块的Initialize方法。
  - AimRT框架保证在主线程中调用模块的Initialize方法，模块不应阻塞Initialize方法太久。
  - AimRT框架在调用模块Initialize方法时，会传入一个CoreRef句柄，模块可以存储此句柄，并在后续通过它调用框架的功能。
  - 在AimRT框架调用模块的Initialize方法之前，保证所有的组件（例如配置、日志等）都已经完成Initialize，但还未Start。
  - 如果模块在Initialize方法中抛了异常，等效于返回false。
  - 如果有任何模块在AimRT框架调用其Initialize方法时返回了false，则整个AimRT框架会Initialize失败。
- `bool Start()`：用于启动模块。
  - AimRT框架在Start阶段依次调用各模块的Start方法。
  - AimRT框架保证在主线程中调用模块的Start方法，模块不应阻塞Start方法太久。
  - 在AimRT框架调用模块的Start方法之前，保证所有的组件（例如配置、日志等）都已经进入Start阶段。
  - 如果模块在Start方法中抛了异常，等效于返回了false。
  - 如果有任何模块在AimRT框架调用其Start方法时返回了false，则整个AimRT框架会Start失败。
- `void Shutdown()`：用于停止模块，一般用于整个进程的优雅退出。
  - AimRT框架在Shutdown阶段依次调用各个模块的Shutdown方法。
  - AimRT框架保证在主线程中调用模块的Shutdown方法，模块不应阻塞Shutdown方法太久。
  - AimRT框架可能在任何阶段直接进入Shutdown阶段。
  - 如果模块在Shutdown方法中抛了异常，框架会catch住并直接返回。
  - 在AimRT框架调用模块的Shutdown方法之后，各个组件（例如配置、日志等）才会Shutdown。


以下是一个简单的示例，实现了一个最基础的HelloWorld模块：
```cpp
#include "aimrt_module_cpp_interface/module_base.h"

class HelloWorldModule : public aimrt::ModuleBase {
 public:
  HelloWorldModule() = default;
  ~HelloWorldModule() override = default;

  ModuleInfo Info() const override {
    return ModuleInfo{.name = "HelloWorldModule"};
  }

  bool Initialize(aimrt::CoreRef core) override { return true; }

  bool Start() override { return true; }

  void Shutdown() override {}
};
```

