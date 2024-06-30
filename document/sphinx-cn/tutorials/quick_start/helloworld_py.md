
# HelloWorld Python

本章将以一个简单的Demo来介绍如何建立一个最基本的AimRT Python工程。


AimRT基于pybind11，在CPP接口层之上包装了一层python接口。本Demo将演示以下几项基本功能：
- 基于pip安装AimRT；
- 基于App模式，直接在Main方法中创建AimRT实例并使用其中的功能；
- 使用基础的日志功能；
- 使用基础的配置功能；
- 运行Python脚本以体验AimRT的功能。


## STEP1：安装AimRT Python包

具体请参考[引用与安装（Python）](installation_py.md)中的步骤。


## STEP2: 编写业务代码

参考以下代码，编写一个python文件`helloworld_app.py`，在其中创建了一个aimrt实例，并解析传入的配置文件、打印一些简单的日志。

```python
import argparse
import threading
import time
import aimrt_py
import aimrt_py_log
import yaml

def main():
    parser = argparse.ArgumentParser(description='Helloworld app.')
    parser.add_argument('--cfg_file_path', type=str, default="", help='config file path')
    args = parser.parse_args()

    # create aimrt core
    core = aimrt_py.Core()

    # init aimrt core
    core_options = aimrt_py.CoreOptions()
    core_options.cfg_file_path = args.cfg_file_path
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


## STEP3: 编写配置文件
以下是一个简单的示例配置文件`helloworld_cfg.yaml`。这个配置文件中的其他内容将在后续章节中介绍，这里关注两个地方：
- `aimrt.log`节点：此处指定了日志的一些细节。
- `HelloWorldModule`节点：此处为`HelloWorldModule`的配置，可以在模块中读取到。


```yaml
aimrt:
  log: # log配置
    core_lvl: INFO # 内核日志等级，可选项：Trace/Debug/Info/Warn/Error/Fatal/Off，不区分大小写
    backends: # 日志backends
      - type: console # 控制台日志
        options:
          color: true # 是否彩色打印
      - type: rotate_file # 文件日志
        options:
          path: ./log # 日志文件路径
          filename: helloworld.log # 日志文件名称
          max_file_size_m: 4 # 日志文件最大尺寸，单位m
          max_file_num: 10 # 最大日志文件数量，0代表无限

# 模块自定义配置，框架会为每个模块生成临时配置文件，开发者通过Configurator接口获取该配置文件路径
HelloWorldModule:
  key1: val1
  key2: val2
```

## STEP4: 启动并测试

将python代码执行文件`helloworld_app.py`和配置文件`helloworld_cfg.yaml`拷贝到一个目录下，然后执行以下命令运行脚本，观察打印出来的日志：
```shell
python3 ./helloworld_app.py --cfg_file_path=./helloworld_cfg.yaml
```

