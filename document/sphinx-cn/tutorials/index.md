# Tutorials


## 快速开始

通过此部分文档，您可以了解到如何引用、安装AimRT，并通过创建一个`Hello World`程序来快速体验AimRT。

```{toctree}
:maxdepth: 1

quick_start/installation_cpp.md
quick_start/installation_py.md
quick_start/helloworld_cpp.md
quick_start/helloworld_py.md
```

## 概念

通过此部分文档，您可以了解到AimRT中的一些核心概念和设计思想。


```{toctree}
:maxdepth: 1

concepts/cmake.md
concepts/concepts.md
concepts/core_design.md
```

## 接口概述

您可以先通过概述章节了解一些通用信息。

```{toctree}
:maxdepth: 1

interface/interface.md
```

## CPP接口文档

您可以通过以下文档了解逻辑开发阶段的C++接口用法。

```{toctree}
:maxdepth: 1

interface_cpp/common.md
interface_cpp/core_ref.md
interface_cpp/module_base.md
interface_cpp/configurator.md
interface_cpp/executor.md
interface_cpp/logger.md
interface_cpp/parameter.md
interface_cpp/channel.md
interface_cpp/rpc.md
```

您可以通过以下文档了解部署运行阶段的C++接口用法。

```{toctree}
:maxdepth: 1

interface_cpp/runtime.md
```



## Python接口文档

<!-- TODO, 本章节待整理-------- -->

您可以通过以下文档了解逻辑开发阶段的Python接口用法。

```{toctree}
:maxdepth: 1

interface_py/common.md
interface_py/core_ref.md
interface_py/module_base.md
interface_py/configurator.md
interface_py/executor.md
interface_py/logger.md
interface_py/channel.md
interface_py/rpc.md
```

您可以通过以下文档了解部署运行阶段的Python接口用法。

```{toctree}
:maxdepth: 1

interface_py/runtime.md
```

## 配置文档

您可以通过以下文档了解详细的配置方法。

```{toctree}
:maxdepth: 1

cfg/common.md
cfg/module.md
cfg/configurator.md
cfg/plugin.md
cfg/main_thread.md
cfg/guard_thread.md
cfg/executor.md
cfg/log.md
cfg/channel.md
cfg/rpc.md
```

## 插件

<!-- TODO, 本章节待整理-------- -->

AimRT提供了大量官方插件，这部分文档将介绍这些插件的功能以及详细的配置选项。

```{toctree}
:maxdepth: 1

plugins/net_plugin.md
plugins/mqtt_plugin.md
plugins/ros2_plugin.md
plugins/sm_plugin.md
plugins/lcm_plugin.md
plugins/parameter_plugin.md
plugins/time_manipulator_plugin.md
plugins/log_control_plugin.md
```

如果开发者想定制开发自己的插件，可以参考以下文档。
```{toctree}
:maxdepth: 1

plugins/how_to_dev_plugin.md
```


## CLI工具

<!-- TODO, 本章节待整理-------- -->

AimRT提供了一个命令行工具，可以帮助开发者快速完成一些操作。

```{toctree}
:maxdepth: 1

cli_tool/cli_tool.md
cli_tool/gen_prj.md
```


## 示例

<!-- TODO, 本章节待整理-------- -->

AimRT提供了详细且全面的示例，开发者可以基于示例进行深入学习。

```{toctree}
:maxdepth: 1

examples/examples.md
```
