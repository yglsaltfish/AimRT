# 时间管理器设计

***TODO待完善***

## 插件概述


**time_manipulator_plugin**中提供了`time_manipulator`执行器和一个运行时控制该执行器表现的RPC：`TimeManipulatorService`，协议文件为[time_manipulator.proto](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/protocols/plugins/time_manipulator_plugin/time_manipulator.proto)。请注意，**time_manipulator_plugin**没有提供任何通信后端，因此本插件中的RPC一般要搭配其他通信插件的RPC通信后端一块使用，例如[net_plugin](./net_plugin.md)中的http RPC后端。


## `time_manipulator` 执行器



## TimeManipulatorService





