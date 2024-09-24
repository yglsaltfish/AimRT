# Zenoh 插件


## 相关链接

参考示例：
- {{ '[zenoh_plugin]({}/src/examples/plugins/zenoh_plugin)'.format(code_site_root_path_url) }}


## 概述

**zenoh_plugin** 是一个轻量级的、高效的、实时的数据传输插件，它旨在为分布式系统提供低延迟、高吞吐量的数据传输和处理能力。此插件为 AimRT 提供以下组件：
- `zenoh` 类型 Channel 后端
- `zenoh` 类型 Rpc 后端
  
## 适用场景
- 需要`服务发现`机制的任务系统
- 灵活的网络拓扑结构
- 低延迟、高吞吐量的网络通信和数据传输

## 配置
插件的配置项如下：

|      节点       |  类型  | 是否可选 | 默认值 |            作用             |
| :-------------: | :----: | :------: | :----: | :-------------------------: |
| native_cfg_path | string |   可选   |   ""   | 使用zenoh提供的原生配置文件 |
|  limit_domain   | string |   可选   |   ""   |   对插件的通信域进行限制    |

关于**zenoh_plugin**的配置，使用注意点如下：
- `native_cfg_path` 表示 zenoh 提供的原生配置文件的路径，如果不填写则默认使用 zenoh 官方提供的默认配置，具体配置内容请参考 zenoh 官方关于[configuration](https://zenoh.io/docs/manual/configuration/)的说明。
- limit_domain 表示插件的通信域（即zenoh的 key ），如果不填写则默认使用插件默认的通信域，只有相`匹配`的通信域才能进行通信，具体的书写格式如下：

```shell

#请以"/"开始，中间以"/"分隔，结尾不要带"/"，如：

/xxx/yyy/zzz/...

```
  最简单的一种匹配就是二者域相同，除此之外，zenoh官方提供了更灵活的匹配机制，具体可查阅zenoh官方关于[key](https://zenoh.io/docs/manual/abstractions/)的解释

以下是一个简单的示例：

```yaml
aimrt:
  plugin:
    plugins:
      - name: zenoh_plugin
        path: ./libaimrt_zenoh_plugin.so #【可选】插件路径。如果是硬编码注册的插件不需要填
        options: 
          native_cfg_path: ./DEFAULT_CONFIG.json5 #【可选】使用zenoh提供的原生配置文件
          limit_domain: /room1/A2  #【可选】限制域
```


## zenoh 类型 Rpc 后端
`zenoh`类型的 Rpc后端是**zenoh_plugin**中提供的一种Rpc后端，主要用来构建请求-响应模型。





## zenoh 类型 Channel 后端

`zenoh`类型的 Channel 后端是**zenoh_plugin**中提供的一种Channel后端，主要用来构建发布和订阅模型。

以下是一个简单的发布端的示例：
```yaml
aimrt:
  plugin:
    plugins:
      - name: zenoh_plugin
        path: ./libaimrt_zenoh_plugin.so
        options: 
          native_cfg_path: ./DEFAULT_CONFIG.json5
          limit_domain: /room1/A2
  channel:
    backends:
      - type: zenoh
    pub_topics_options:
      - topic_name: "(.*)" 
        enable_backends: [zenoh]
```


以下是一个简单的订阅端的示例：
```yaml
aimrt:
  plugin:
    plugins:
      - name: zenoh_plugin
        path: ./libaimrt_zenoh_plugin.so
        options: 
          native_cfg_path: ./DEFAULT_CONFIG.json5
          limit_domain: /room1/A2
channel:
    backends:
      - type: zenoh
    sub_topics_options:
      - topic_name: "(.*)"
        enable_backends: [zenoh]
```

以上示例中都使用 zeonh 的服务发现机制，即在统一网络中的两个端点可自动发现彼此并建立连接，因此在配置过程中用户可以不用手动输入ip地址等信息，降低使用复杂度。

在这个过程中，底层使用的 Topic 名称格式为：`channel/${topic_name}/${message_type}${limit_domain}`。其中，`${topic_name}`为 AimRT 的 Topic 名称，`${message_type}`为 url 编码后的 AimRT 消息名称， `${limit_domain}`为插件的限制域。这个 Topic 被设置成为 Zenoh 最终的键表达式（Keyxpr），这是 Zenoh 的提供的资源标识符，只有键表达式匹配的订阅者和发布者才能够进行通信。

例如，AimRT Topic 名称为`test_topic`，消息类型为`pb:aimrt.protocols.example.ExampleEventMsg`，限制域为`/room1/A2`，则最终 Zenoh 的 topic 名称为：`channel/test_topic/pb%3Aaimrt.protocols.example.ExampleEventMsg/room1/A2`。如果订阅者和发布者的 Topic 均为`channel/test_topic/pb%3Aaimrt.protocols.example.ExampleEventMsg/room1/A2`，则二者可以进行通信。

在AimRT发布端发布数据到订阅端这个链路上，Zenoh 数据包格式整体分 3 段：
- 序列化类型，一般是`pb`或`json`
- context 区
  - context 数量，1 字节，最大 255 个 context
  - context_1 key, 2 字节长度 + 数据区
  - context_2 key, 2 字节长度 + 数据区
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
