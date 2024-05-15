
# ReleaseNotes

[TOC]

## v0.6.0（开发中）

- 修复了topic/rpc规则配置的问题，现在是以第一个命中的规则为准，命中后就不会再管后续规则了；
- 去除了各个插件的单独的enable配置；
- 大幅调整了example体系；
- 重构了文档体系，大幅完善了文档；
- 优化了main_executor的性能；
- 提供了rpc/channel统一的backend开关；
- 提供了每个Module独立的enable开关；
- ros2_plugin：
  - ros2 RPC Backend现在支持非ros2协议；
  - ros2 Channel Backend现在支持非ros2协议；
- mqtt_plugin:
  - 修复了mqtt插件的一些问题；

