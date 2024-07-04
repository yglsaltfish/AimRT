# ModuleBase

## ModuleBase类型概述


业务模块需要继承`ModuleBase`基类，它定义了业务模块所需要实现的几个接口，包括：
- `Info()->ModuleInfo`：用于AimRT框架获取模块信息，包括模块名称、模块版本等。
  - AimRT框架会在加载模块时调用此接口，读取模块信息。
  - `ModuleInfo`结构中除`name`是必须项，其余都是可选项。
  - 如果模块在其中抛了异常，等效于返回一个空ModuleInfo。
- `Initialize(CoreRef)->bool`：用于初始化模块。
  - AimRT框架在Initialize阶段，依次调用各模块的Initialize方法。
  - AimRT框架保证在主线程中调用模块的Initialize方法，模块不应阻塞Initialize方法太久。
  - AimRT框架在调用模块Initialize方法时，会传入一个CoreRef句柄，模块可以存储此句柄，并在后续通过它调用框架的功能。
  - 在AimRT框架调用模块的Initialize方法之前，保证所有的组件（例如配置、日志等）都已经完成Initialize，但还未Start。
  - 如果模块在Initialize方法中抛了异常，等效于返回false。
  - 如果有任何模块在AimRT框架调用其Initialize方法时返回了false，则整个AimRT框架会Initialize失败。
- `Start()->bool`：用于启动模块。
  - AimRT框架在Start阶段依次调用各模块的Start方法。
  - AimRT框架保证在主线程中调用模块的Start方法，模块不应阻塞Start方法太久。
  - 在AimRT框架调用模块的Start方法之前，保证所有的组件（例如配置、日志等）都已经进入Start阶段。
  - 如果模块在Start方法中抛了异常，等效于返回了false。
  - 如果有任何模块在AimRT框架调用其Start方法时返回了false，则整个AimRT框架会Start失败。
- `Shutdown()`：用于停止模块，一般用于整个进程的优雅退出。
  - AimRT框架在Shutdown阶段依次调用各个模块的Shutdown方法。
  - AimRT框架保证在主线程中调用模块的Shutdown方法，模块不应阻塞Shutdown方法太久。
  - AimRT框架可能在任何阶段直接进入Shutdown阶段。
  - 如果模块在Shutdown方法中抛了异常，框架会catch住并直接返回。
  - 在AimRT框架调用模块的Shutdown方法之后，各个组件（例如配置、日志等）才会Shutdown。



`ModuleInfo`类型有以下成员，其中除了`name`是必选的，其他都是可选项：
- name(str)
- major_version(int)
- minor_version(int)
- patch_version(int)
- build_version(int)
- author(str)
- description(str)


  
以下是一个简单的示例，实现了一个最基础的HelloWorld模块：
```python
import aimrt_py

class HelloWorldModule(aimrt_py.ModuleBase):
    def __init__(self):
        super().__init__()

    # 返回本模块的一些信息
    def Info(self):
        info = aimrt_py.ModuleInfo()
        info.name = "HelloWorldModule"
        return info

    # 框架初始化时会调用此方法
    def Initialize(self, core):
        print("Initialize")
        return True

    # 框架启动时会调用此方法
    def Start(self):
        print("Start")
        return True

    # 框架停止时会调用此方法
    def Shutdown(self):
        print("Shutdown")
```

