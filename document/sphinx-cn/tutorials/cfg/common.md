# 配置概述

AimRT进程在启动时需要一个配置文件，来定义各个组件的运行时表现。


## Yaml格式

AimRT采用Yaml作为配置文件格式。YAML 是一种人类可读的数据序列化语言，通常用于编写配置文件。关于Yaml的语法细节，请参考互联网上的文档或[Yaml官方网站](https://yaml.org/)。


## 不区分业务开发语言

AimRT的配置文件不区分业务代码的开发语言，无论是Python还是Cpp都使用同一套配置标准。


## AimRT框架配置的基本结构

AimRT的配置文件中，所有的框架相关配置都在`aimrt`根节点下。在`aimrt`根节点下包含各个组件的配置节点，基本书写风格是小写字母+下划线，目前共有**10**个主要组件可以配置，且所有的组件配置都是可选的：


| 节点            |   作用 |
| ----            | ---- |
| configurator    |  配置工具的配置 |
| plugin          |  插件配置 |
| main_thread     |  主线程配置 |
| allocator       |  内存分配器配置 |
| executor        |  执行器配置 |
| log             |  日志配置 |
| rpc             |  RPC配置 |
| channel         |  Channel配置 |
| parameter       |  参数配置 |
| module          |  模块配置 |

以下是一个简单的示例，先给读者一个感性的印象。关于各个组件的详细配置方法，请参考后续章节：
```yaml
aimrt:
  configurator:
    temp_cfg_path: ./cfg/tmp
  plugin:
    plugins:
      - name: xxx_plugin
        path: ./libaimrt_mqtt_plugin.so
  main_thread:
    name: main_thread
  allocator:
    # ...
  executor:
    executors:
      - name: work_executor
        type: asio_thread
  log:
    core_lvl: INFO
    default_module_lvl: INFO
    backends:
      - type: console
  rpc:
    backends:
      - type: local
      - type: mqtt
    clients_options:
      - func_name: "(.*)"
        enable_backends: [local]
    servers_options:
      - func_name: "(.*)"
        enable_backends: [local]
  channel:
    backends:
      - type: local
      - type: mqtt
    pub_topics_options:
      - topic_name: "(.*)"
        enable_backends: [local]
    sub_topics_options:
      - topic_name: "(.*)"
        enable_backends: [local]
  parameter:
    # ...
  module:
    pkgs:
      - path: /path/to/libxxx_pkg.so
    modules:
      - name: FooModule
        enable: True
        log_lvl: INFO
        cfg_file_path: /path/to/foo_module_cfg.yaml
      - name: BarModule
        log_lvl: WARN
```


## 业务配置

除了框架的配置，AimRT还支持用户将业务模块的配置也以Yaml的形式写在同一个配置文件中，以模块名称为节点名，示例如下：
```yaml
aimrt:
  # ...

# Module custom configuration, with module name as node name
FooModule:
  key_1: val_1
  key_2: val_2

BarModule:
  xxx_array:
    - val1
    - val2
  xxx_map:
    key_1: val_1
    key_2: val_2

```

当然，如果用户不想要把业务模块配置与AimRT框架配置写在一个文件中，甚至不想要以Yaml格式来写配置，AimRT也可以支持。具体的使用方式请参考`aimrt.configurator`的文档。


## 环境变量替换功能

AimRT的配置文件支持替换环境变量。在解析配置文件前，AimRT会将配置文件中形如`${XXX_ENV}`的字符串替换为环境变量`XXX_ENV`的值。注意，如果没有此环境变量，则会替换为字符串`null`。


## 配置文件Dump功能

如果使用者不确定自己的配置是否正确，可以使用AimRT的`配置Dump功能`，将AimRT解析后的完整配置文件Dump下来，看看和开发者自己的预期是否相符。具体可以参考[CPP运行时接口](../interface_cpp/runtime.md)中关于启动参数的章节。


