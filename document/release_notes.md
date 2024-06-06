
# ReleaseNotes

[TOC]

## v0.6.0

- 修复了topic/rpc规则配置的问题，现在是以第一个命中的规则为准，命中后就不会再管后续规则了；
- 去除了各个插件的单独的enable配置；
- 大幅调整了example体系；
- 重构了文档体系，大幅完善了文档；
- 优化了main_executor的性能；
- 提供了rpc/channel统一的backend开关；
- 提供了每个Module独立的enable开关；
- 提供了同步和异步的RPC接口，以及相关example；
- 原来的协程RPC Proxy/Service接口重命名为CoProxy/CoService，之前老的命名将在后几个版本中删除；
- 优化了框架日志，提供【Initialization Report】，现在可以在启动日志中查看executor信息、channel/rpc注册信息；
- consoler/file日志后端现在支持模块过滤功能，并且允许同时注册多个file日志后端，方便将不同模块的日志打印到不同文件中；
- ros2_plugin：
  - ros2 RPC Backend现在支持非ros2协议；
  - ros2 Channel Backend现在支持非ros2协议；
- mqtt_plugin:
  - 修复了mqtt插件的一些问题；

## v0.7.0 (开发中)

- App模式下支持直接create模块；




