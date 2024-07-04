# Configurator

## ConfiguratorRef句柄概述


模块可以通过调用`CoreRef`句柄的`GetConfigurator()`接口，获取`ConfiguratorRef`句柄，通过其使用一些配置相关的功能。其提供的核心接口如下：
- `GetConfigFilePath()->str` : 获取配置文件路径


使用注意点如下：
- `GetConfigFilePath()->str`接口：用于获取模块配置文件的路径。
  - 请注意，此接口仅返回一个模块配置文件的路径，模块开发者需要自己读取配置文件并解析。
  - 这个接口具体会返回什么样的路径，请参考部署运行阶段[aimrt.module 配置文档](../cfg/module.md)。


一个简单的使用示例如下：
```python
import aimrt_py
import yaml

class HelloWorldModule(aimrt_py.ModuleBase):
    def Initialize(self, core):
        # 获取配置句柄
        configurator = core.GetConfigurator()

        # 获取配置路径
        cfg_file_path = configurator.GetConfigFilePath()

        # 根据用户实际使用的文件格式来解析配置文件。本例中基于yaml来解析
        with open(cfg_file_path, 'r') as file:
            data = yaml.safe_load(file)
            # ...

        return True
```
