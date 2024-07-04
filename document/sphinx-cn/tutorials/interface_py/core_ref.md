# CoreRef

## CoreRef句柄概述

在模块的`Initialize`方法中，AimRT框架会传入一个`CoreRef`句柄，模块通过该句柄的一些接口调用框架的功能。`CoreRef`中提供的核心接口如下：
- `GetConfigurator()->ConfiguratorRef` : 获取配置句柄
- `GetLogger()->LoggerRef` ： 获取日志句柄
- `GetExecutorManager()->ExecutorManagerRef` ： 获取执行器句柄
- `GetRpcHandle()->RpcHandleRef` ： 获取RPC句柄
- `GetChannelHandle()->ChannelHandleRef` ： 获取Channel句柄


关于`CoreRef`的使用注意点如下：
- AimRT框架会为每个模块生成一个专属`CoreRef`句柄，以实现资源隔离、监控等方面的功能。
- 模块通过`CoreRef`中的接口获取对应组件的句柄，并通过它们来调用相关功能。

一个简单的示例如下：
```python
import aimrt_py
import aimrt_py_log

class HelloWorldModule(aimrt_py.ModuleBase):
    def Initialize(self, core):
        # 获取日志句柄
        logger = core.GetLogger()

        # 使用日志句柄打印日志
        aimrt_py_log.info(logger, "This is a test log")
        return True
```
