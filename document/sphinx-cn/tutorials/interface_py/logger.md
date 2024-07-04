# Logger


## LoggerRef：日志句柄

AimRT提供了`aimrt_py_log`包，来封装log接口，其作用相当于CPP接口中的日志宏。在AimRT中，模块可以通过调用`CoreRef`句柄的`GetLogger()`接口，获取`LoggerRef`句柄，这个句柄可以直接作为`aimrt_py_log`中日志接口的参数，来实现日志功能。


模块开发者可以直接参照以下示例的方式，使用分配给模块的日志句柄来打印日志：
```python
import aimrt_py
import aimrt_py_log

class HelloWorldModule(aimrt_py.ModuleBase):
    def Initialize(self, core):
        # 获取日志
        logger = core.GetLogger()

        # 打印日志
        aimrt_py_log.trace(logger, "This is a test trace log")
        aimrt_py_log.debug(logger, "This is a test trace log")
        aimrt_py_log.info(logger, "This is a test trace log")
        aimrt_py_log.warn(logger, "This is a test trace log")
        aimrt_py_log.error(logger, "This is a test trace log")
        aimrt_py_log.fatal(logger, "This is a test trace log")

        return True
```
