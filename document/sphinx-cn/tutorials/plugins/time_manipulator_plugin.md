# 时间管理器插件


## 相关链接

协议文件：
- {{ '[time_manipulator.proto]({}/src/protocols/plugins/time_manipulator_plugin/time_manipulator.proto)'.format(code_site_root_path_url) }}

参考示例：
- {{ '[time_manipulator_plugin]({}/src/examples/plugins/time_manipulator_plugin)'.format(code_site_root_path_url) }}


## 插件概述


**time_manipulator_plugin**中提供了`time_manipulator`执行器，并注册了一个基于 protobuf 协议定义的 RPC，提供了对于`time_manipulator`执行器的一些管理接口。请注意，**time_manipulator_plugin**没有提供任何通信后端，因此本插件一般要搭配其他通信插件的 RPC 后端一块使用，例如[net_plugin](./net_plugin.md)中的 http RPC 后端。


在当前版本，本插件没有插件级的配置。以下是一个简单的配置示例，将**time_manipulator_plugin**与**net_plugin**中的 http RPC 后端搭配使用：

```yaml
aimrt:
  plugin:
    plugins:
      - name: net_plugin
        path: ./libaimrt_net_plugin.so
        options:
          thread_num: 4
          http_options:
            listen_ip: 127.0.0.1
            listen_port: 50080
      - name: time_manipulator_plugin
        path: ./libaimrt_time_manipulator_plugin.so
  rpc:
    backends:
      - type: http
    servers_options:
      - func_name: "(pb:/aimrt.protocols.time_manipulator_plugin.*)"
        enable_backends: [http]
```


## time_manipulator 执行器


`time_manipulator`执行器是一种基于时间轮实现的可调速执行器，一般用于仿真、模拟等有定时任务、且对定时精度要求不高的场景，例如 RPC 超时处理。它会启动一个单独的线程跑时间轮，同时也支持将具体的任务投递到其他执行器中执行。其所有的配置项如下：


## TimeManipulatorService





