# Zenoh插件

## 插件概述

**zenoh_plugin**是一个轻量级的、高效的、实时的数据传输插件，它旨在为分布式系统提供低延迟、高吞吐量的数据传输和处理能力。此插件为AimRT提供以下组件：
- `zenoh`类型Channel后端
- `zenoh`类型RPC后端(暂未实现)

插件的配置项如下：

（暂无需要用户自定义配置选项）

由于zenoh自带的服务发现机制，用户不必手动配置IP和端口。目前暂未开放任何配置选项，后续会增加用于网络控制的配置选项。

以下是一个简单的示例：

```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: zenoh_plugin # 【必选】插件名称
        path: ./libaimrt_zenoh_plugin.so #【可选】插件路径。如果是硬编码注册的插件不需要填
```
**zenoh_plugin**插件基于插件基于[zenoh.c](https://github.com/eclipse-zenoh/zenoh-c)开发的的。`请注意`，该插件在编译过程依赖于[Rust](https://www.rust-lang.org/)，请确保运行环境中存在Rust编译器，否则该插件将自动关闭，尽管在编译选项中是开启状态。

## `zenoh` Channel后端

`zenoh`类型的Channel后端是**zenoh_plugin**中提供的一种Channel后端，主要用来构建发布和订阅模型。其所有的配置项如下：

（暂无需要用户自定义配置选项）

以下是一个简单的发布端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: zenoh_plugin # 【必选】插件名称
        path: ./libaimrt_zenoh_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: zenoh # 【必选】Channel后端类型
    pub_topics_options: # 【可选】Channel Pub Topic配置
      - topic_name: "(.*)"  # 【必选】Channel Pub Topic名称，支持正则表达式
        enable_backends: [zenoh ] # 【必选】Channel Pub Topic允许使用的Channel后端列表
```


以下是一个简单的订阅端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: zenoh_plugin # 【必选】插件名称
        path: ./libaimrt_zenoh_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: zenoh # 【必选】Channel后端类型
    sub_topics_options: # 【可选】Channel Sub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Sub Topic名称，支持正则表达式
        enable_backends: [zenoh] # 【必选】Channel Sub Topic允许使用的Channel后端列表
```
以上示例中都使用zeonh的服务发现机制，即在统一网络中的两个端点可自动发现彼此并建立连接，因此在配置过程中不需要用户手动输入任何参数，降低使用复杂度。

在这个过程中，底层使用的Zenoh Topic名称格式为：`channel/${topic_name}/${message_type}`。其中，`${topic_name}`为AimRT的Topic名称，`${message_type}`为url编码后的AimRT消息名称。这个Topic被设置成为Zenoh的键表达式（Keyxpr）,这是Zenoh的提供的资源标识符，只有键表达式匹配的订阅者和发布者才能够进行通信。

例如，AimRT Topic名称为`test_topic`，消息类型为`pb:aimrt.protocols.example.ExampleEventMsg`，则最终Zenoh的topic名称为：`channel/test_topic/pb%3Aaimrt.protocols.example.ExampleEventMsg`。如果订阅者和发布者的Topic均为`channel/test_topic/pb%3Aaimrt.protocols.example.ExampleEventMsg`，则二者可以进行通信。

在AimRT发布端发布数据到订阅端这个链路上，Zenoh数据包格式整体分3段：
- 序列化类型，一般是`pb`或`json`
- context区
  - context数量，1字节，最大255个context
  - context_1 key, 2字节长度 + 数据区
  - context_2 key, 2字节长度 + 数据区
  - ...
- 数据

数据包格式如下：
```
| n(0~255) [1 byte] | content type [n byte]
| context num [1 byte]
| context_1 key size [2 byte] | context_1 key data [key_1_size byte]
| context_1 val size [2 byte] | context_1 val data [val_1_size byte]
| context_2 key size [2 byte] | context_2 key data [key_2_size byte]
| context_2 val size [2 byte] | context_2 val data [val_2_size byte]
| ...
| msg data [len - 1 - n byte] |
```