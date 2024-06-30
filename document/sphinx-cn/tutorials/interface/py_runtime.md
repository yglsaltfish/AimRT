
# 运行时接口-Python版本


## 简介

与CPP接口不一样的是，AimRT Python接口只提供**App模式**，开发者需要自行管理python中的main方法，并在其中创建、管理AimRT Core实例。在**App模式**模式下，AimRT Python接口与CPP接口类似，提供了**注册**或**创建**这两种方式去开发用户的模块逻辑。

注意，如未作特殊说明，本文档中所有的类型都是`aimrt_py`包中的。

## Core类型以及其基本方法

开发者需要先创建一个`aimrt_py`包中的`Core`实例，该类型提供了以下几个关键的运行时方法：
- `Initialize(core_options)`: 初始化AimRT运行时；
- `Start()`: 启动AimRT运行时，注意，该方法将阻塞当前线程，直到在其他线程中调用了`Shutdown`方法；
- `Shutdown()`: 停止AimRT运行时，支持重入；
- `RegisterModule(module)`: 注册一个模块
- `CreateModule(module_name)->module_handle`: 创建一个模块


前三个方法是AimRT实例的运行控制方法，后两个方法则对应了**注册**和**创建**这两种开发用户的模块逻辑的方式。其中，`Initialize`方法接收一个`CoreOptions`类型作为参数。此类型包含以下几个成员：
- `cfg_file_path`：str，配置文件路径
- `dump_cfg_file`：bool，是否需要dump实际配置文件，默认false
- `dump_cfg_file_path`：str，dump配置文件的路径，如果不配置则默认生成在当前目录下


以下是一个简单的示例，该示例启动了一个AimRT运行时，但没有加载任何业务逻辑：
```python
import threading
import time
import aimrt_py
import aimrt_py_log

def main():
    # create aimrt core
    core = aimrt_py.Core()

    # init aimrt core
    core_options = aimrt_py.CoreOptions()
    core_options.cfg_file_path = "path/to/cfg/xxx_cfg.yaml"
    core.Initialize(core_options)

    # start aimrt core
    thread = threading.Thread(target=core.Start)
    thread.start()

    # shutdown aimrt core
    time.sleep(1)
    core.Shutdown()

    thread.join()

if __name__ == '__main__':
    main()
```


## 注册模块

在注册模式下，开发者需要继承`ModuleBase`基类来创建一个自己的`Module`，并实现其中的`Initialize`、`Start`等方法，然后在`Core`实例调用`Initialize`方法之前将该`Module`注册到`Core`实例中。在此方式下仍然有一个比较清晰的`Module`边界。


以下是一个简单的例子，展示了如何编写一个自己的模块，并注册到`Core`实例中：
```python
import threading
import signal
import sys
import aimrt_py
import aimrt_py_log
import yaml

# 继承 aimrt_py.ModuleBase 实现一个Module
class HelloWorldModule(aimrt_py.ModuleBase):
    def __init__(self):
        super().__init__()
        self.core = aimrt_py.CoreRef()
        self.logger = aimrt_py.LoggerRef()

    def Info(self):
        info = aimrt_py.ModuleInfo()
        info.name = "HelloWorldModule"
        return info

    def Initialize(self, core):
        self.core = core
        self.logger = self.core.GetLogger()

        # log
        aimrt_py_log.info(self.logger, "Module initialize")

        try:
            # configure
            module_cfg_file_path = self.core.GetConfigurator().GetConfigFilePath()
            with open(module_cfg_file_path, 'r') as file:
                data = yaml.safe_load(file)
                aimrt_py_log.info(self.logger, str(data))

        except Exception as e:
            aimrt_py_log.error(self.logger, "Initialize failed. {}".format(e))
            return False

        return True

    def Start(self):
        aimrt_py_log.info(self.logger, "Module start")

        return True

    def Shutdown(self):
        aimrt_py_log.info(self.logger, "Module shutdown")


global_core = None
def signal_handler(sig, frame):
    global global_core
    if (global_core and (sig == signal.SIGINT or sig == signal.SIGTERM)):
        global_core.Shutdown()
        return

    sys.exit(0)


def main():
    # 注册ctrl-c信号
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    print("AimRT start.")

    # 创建 aimrt_py.Core 实例
    core = aimrt_py.Core()

    global global_core
    global_core = core

    # 注册模块
    module = HelloWorldModule()
    core.RegisterModule(module)

    # 初始化 aimrt_py.Core 实例
    core_options = aimrt_py.CoreOptions()
    core_options.cfg_file_path = "path/to/cfg/xxx_cfg.yaml"
    core.Initialize(core_options)

    # 启动 aimrt_py.Core 实例
    thread = threading.Thread(target=core.Start)
    thread.start()

    # 等待停止
    while thread.is_alive():
        thread.join(1.0)

    core.Shutdown()

    global_core = None

    print("AimRT exit.")

if __name__ == '__main__':
    main()
```


## 创建模块


在`Core`实例调用`Initialize`方法之后，通过`CreateModule`可以创建一个模块，并返回一个句柄，开发者可以直接基于此句柄调用一些框架的方法，比如RPC或者Log等。但这些方法仍然需要满足在[模块接口](py_module.md)文档中所定义的、只能在特定阶段执行等相关规定。在此方式下没有一个比较清晰的`Module`边界，一般仅用于快速做一些小工具。

以下是一个简单的例子，开发者需要编写的Python文件如下：

```python
import argparse
import threading
import time
import aimrt_py
import aimrt_py_log
import yaml

def main():
    # create aimrt core
    core = aimrt_py.Core()

    # init aimrt core
    core_options = aimrt_py.CoreOptions()
    core_options.cfg_file_path = "path/to/cfg/xxx_cfg.yaml"
    core.Initialize(core_options)

    # create module handle
    module_handle = core.CreateModule("HelloWorldModule")

    # use cfg and log
    module_cfg_file_path = module_handle.GetConfigurator().GetConfigFilePath()
    with open(module_cfg_file_path, 'r') as file:
        data = yaml.safe_load(file)
        key1 = str(data["key1"])
        key2 = str(data["key2"])
        aimrt_py_log.info(module_handle.GetLogger(), "key1: {}, key2: {}.".format(key1, key2))

    # start aimrt core
    thread = threading.Thread(target=core.Start)
    thread.start()
    time.sleep(1)

    # use log
    count = 0
    while(count < 10):
        count = count + 1
        aimrt_py_log.info(module_handle.GetLogger(), "Conut : {}.".format(count))

    # shutdown aimrt core
    time.sleep(1)
    core.Shutdown()

    thread.join()

if __name__ == '__main__':
    main()

```



