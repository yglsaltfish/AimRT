
# Mqtt插件

[TOC]

## 插件概述

&emsp;&emsp;**mqtt_plugin**是一个基于mqtt协议实现的网络传输插件，此插件为AimRT提供了以下组件：
- `mqtt`类型RPC后端
- `mqtt`类型Channel后端



&emsp;&emsp;插件的配置项如下：

| 节点              | 类型      | 是否可选| 默认值 | 作用 |
| ----              | ----      | ----  | ----    | ---- |
| broker_addr       | string    | 必选  | ""      | mqtt broker的地址 |
| client_id         | string    | 必选  | ""      | 本节点的mqtt client id |
| max_pkg_size_k    | int       | 可选  | 1024    | 最大包尺寸，单位：KB |


&emsp;&emsp;以下是一个简单的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: mqtt_plugin # 【必选】插件名称
        path: ./libmqtt_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          broker_addr: tcp://127.0.0.1:1883 # 【必选】mqtt broker的地址
          client_id: example_mqtt_client # 【必选】本节点的mqtt client id
          max_pkg_size_k: 1024 # 【可选】最大包尺寸，单位：KB
```


&emsp;&emsp;关于**mqtt_plugin**的配置，使用注意点如下：
- `broker_addr`表示mqtt broker的地址，使用者必须保证有mqtt的broker运行在该地址，否则启动会失败。
- `client_id`表示本节点连接mqtt broker时的client id。
- `max_pkg_size_k`表示传输数据时的最大包尺寸，默认1MB。注意，必须broker也要支持该尺寸才行。


&emsp;&emsp;**mqtt_plugin**插件基于[paho.mqtt.c](https://github.com/eclipse/paho.mqtt.c)封装，在使用时，Channel订阅回调、RPC Server处理方法、RPC Client返回时，使用的都是**paho.mqtt.c**提供的执行器，当使用者在回调中阻塞了线程时，有可能导致无法继续接收/发送消息。正如Module接口文档中所述，一般来说，如果回调中的任务非常轻量，那就可以直接在回调里处理；但如果回调中的任务比较重，那最好调度到其他专门执行任务的执行器里处理。



## `mqtt` RPC后端


&emsp;&emsp;`mqtt`类型的RPC后端是**mqtt_plugin**中提供的一种RPC后端，用于通过mqtt的方式来调用和处理AimRT RPC请求。其所有的配置项如下：


| 节点                          | 类型      | 是否可选| 默认值 | 作用 |
| ----                          | ----      | ----  | ----  | ---- |
| timeout_executor              | string    | 可选  | ""    | Client端RPC超时情况下的执行器 |


&emsp;&emsp;以下是一个简单的客户端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: mqtt_plugin # 【必选】插件名称
        path: ./libmqtt_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          broker_addr: tcp://127.0.0.1:1883 # 【必选】mqtt broker的地址
          client_id: example_client # 【必选】本节点的mqtt client id
          max_pkg_size_k: 1024 # 【可选】最大包尺寸，单位：KB
  executor:
      - name: timeout_handle
        type: time_wheel
  rpc: # 【可选】RPC配置根节点
    backends: # 【可选】RPC后端列表
      - type: mqtt # 【必选】RPC后端类型
        options: # 【可选】RPC Client配置
          clients_options: # 【可选】客户端发起RPC请求时的规则
            - timeout_executor: timeout_handle # 【可选】Client端RPC超时情况下的执行器
    clients_options: # 【可选】RPC Client配置
      - func_name: "(.*)" # 【必选】RPC Client名称，支持正则表达式
        enable_backends: [mqtt] # 【必选】RPC Client允许使用的RPC后端列表
```

&emsp;&emsp;以下则是一个简单的服务端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: mqtt_plugin # 【必选】插件名称
        path: ./libmqtt_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          broker_addr: tcp://127.0.0.1:1883 # 【必选】mqtt broker的地址
          client_id: example_server # 【必选】本节点的mqtt client id
          max_pkg_size_k: 1024 # 【可选】最大包尺寸，单位：KB
  rpc: # 【可选】RPC配置根节点
    backends: # 【可选】RPC后端列表
      - type: mqtt # 【必选】RPC后端类型
    servers_options: # 【可选】RPC Server配置
      - func_name: "(.*)" # 【必选】RPC Server名称，支持正则表达式
        enable_backends: [mqtt] # 【必选】RPC Server允许使用的RPC后端列表
```

&emsp;&emsp;以上示例中，Client端和Server端都连上了`tcp://127.0.0.1:1883`这个地址的一个Mqtt broker，Client端也配置了所有的RPC请求都通过mqtt后端进行处理，从而完成RPC的调用闭环。


&emsp;&emsp;在这个过程中，底层使用的Mqtt Topic名称格式如下：
- Server端
  - 订阅Req使用的topic：`$share/aimrt/aimrt_rpc_req/${func_name}`
  - 发布Rsp使用的topic：`aimrt_rpc_rsp/${client_id}/${func_name}`
- Client端
  - 发布Req使用的topic：`aimrt_rpc_req/${func_name}`
  - 订阅Rsp使用的topic：`aimrt_rpc_rsp/${client_id}/${func_name}`

&emsp;&emsp;其中`${client_id}`是Client端需要保证在同一个Mqtt broker环境下全局唯一的一个值，一般使用在Mqtt broker处注册的client_id。`${func_name}`是url编码后的AimRT RPC方法名称。Server端订阅使用共享订阅，保证只有一个服务端处理请求。此项特性需要支持Mqtt5.0协议的Broker。


&emsp;&emsp;例如，client端向Mqtt broker注册的id为`example_client`，func名称为`/aimrt.protocols.example.ExampleService/GetBarData`，则`${client_id}`值为`example_client`，`${func_name}`值为`%2Faimrt.protocols.example.ExampleService%2FGetBarData`。



&emsp;&emsp;Client -> Server的Mqtt数据包格式整体分4段:
- 序列化类型，一般是`pb`或`json`
- client端想要server端回复rsp的mqtt topic名称。client端自己需要订阅这个mqtt topic
- msg id，4字节，server端会原封不动的封装到rsp包里，供client端定位rsp对应哪个req
- 数据

```
| n(0~255) [1 byte] | content type [n byte]
| m(0~255) [1 byte] | rsp topic name [m byte]
| msg id [4 byte]
| msg data [len - 1 - n - 1 - m - 4 byte]
```

&emsp;&emsp;Server -> Client的Mqtt数据包格式也是整体分4段:
- 序列化类型，一般是`pb`或`json`
- msg id，4字节，req中的msg id
- status code，4字节，框架错误码，如果这个部分不为零，则代表服务端发生了错误，数据段将没有内容
- 数据

```
| n(0~255) [1 byte] | content type [n byte]
| msg id [4 byte]
| status code [4 byte]
| msg data [len - 1 - n - 4 -2 byte]
```


## `mqtt` Channel后端


&emsp;&emsp;`mqtt`类型的Channel后端是**mqtt_plugin**中提供的一种Channel后端，用于通过mqtt的方式来发布和订阅消息。其所有的配置项如下：


| 节点                                  | 类型    | 是否可选| 默认值 | 作用 |
| ----                                  | ----    | ----  | ----  | ---- |
| pub_topics_options                    | array   | 可选  | []    | 发布Topic时的规则 |
| pub_topics_options[i].topic_name      | string  | 必选  | ""    | Topic名称，支持正则表达式 |
| pub_topics_options[i].qos             | int     | 必选  | 2    | Mqtt QOS等级 |
| sub_topics_options                    | array   | 可选  | []    | 发布Topic时的规则 |
| sub_topics_options[i].topic_name      | string  | 必选  | ""    | Topic名称，支持正则表达式 |
| sub_topics_options[i].qos             | int     | 必选  | 2    | Mqtt QOS等级 |


&emsp;&emsp;以下是一个简单的发布端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: mqtt_plugin # 【必选】插件名称
        path: ./libmqtt_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          broker_addr: tcp://127.0.0.1:1883 # 【必选】mqtt broker的地址
          client_id: example_publisher # 【必选】本节点的mqtt client id
          max_pkg_size_k: 1024 # 【可选】最大包尺寸，单位：KB
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: mqtt # 【必选】Channel后端类型
        options: # 【可选】具体Channel后端的配置
          pub_topics_options: # 【可选】发布Topic时的规则
            - topic_name: "(.*)" # 【必选】Topic名称，支持正则表达式
              qos: 2 # 【必选】Mqtt QOS等级
    pub_topics_options: # 【可选】Channel Pub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Pub Topic名称，支持正则表达式
        enable_backends: [mqtt] # 【必选】Channel Pub Topic允许使用的Channel后端列表
```

&emsp;&emsp;以下则是一个简单的订阅端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: mqtt_plugin # 【必选】插件名称
        path: ./libmqtt_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          broker_addr: tcp://127.0.0.1:1883 # 【必选】mqtt broker的地址
          client_id: example_subscriber # 【必选】本节点的mqtt client id
          max_pkg_size_k: 1024 # 【可选】最大包尺寸，单位：KB
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: mqtt # 【必选】Channel后端类型
    sub_topics_options: # 【可选】Channel Sub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Sub Topic名称，支持正则表达式
        enable_backends: [mqtt] # 【必选】Channel Sub Topic允许使用的Channel后端列表
```

&emsp;&emsp;以上示例中，发布端和订阅端都连上了`tcp://127.0.0.1:1883`这个地址的一个Mqtt broker，发布端也配置了所有的消息都通过mqtt后端进行处理，订阅端也配置了所有消息都可以从mqtt后端触发回调，从而打通消息发布订阅的链路。


&emsp;&emsp;在这个过程中，底层使用的Mqtt Topic名称格式为：`/channel/${topic_name}/${message_type}`。其中，`${topic_name}`为AimRT的Topic名称，`${message_type}`为url编码后的AimRT消息名称。

&emsp;&emsp;例如，AimRT Topic名称为`test_topic`，消息类型为`pb:aimrt.protocols.example.ExampleEventMsg`，则最终Mqtt的topic名称为：`/channel/test_topic/pb%3Aaimrt.protocols.example.ExampleEventMsg`。


&emsp;&emsp;在AimRT发布端发布数据到订阅端这个链路上，Mqtt数据包格式整体分两段：
- 序列化类型，一般是`pb`或`json`
- 数据

```
| n(0~255) [1 byte] | content type [n byte] | msg data [len - 1 - n byte] |
```