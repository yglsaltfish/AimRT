
# 网络插件



## 插件概述

**net_plugin**是一个基于boost asio/beast库实现的网络传输插件，此插件为AimRT提供了以下组件：
- `http`类型RPC后端
- `http`类型Channel后端
- `tcp`类型Channel后端
- `udp`类型Channel后端

插件的配置项如下：

| 节点                      | 类型      | 是否可选| 默认值 | 作用 |
| ----                      | ----      | ----  | ----      | ---- |
| thread_num                | int       | 必选  | 2         | net插件需要使用的线程数 |
| http_options              | map       | 可选  | -         | http相关选项 |
| http_options.listen_ip    | string    | 可选  | "0.0.0.0" | http监听IP |
| http_options.listen_port  | int       | 必选  | -         | http监听端口，端口不能被占用 |
| tcp_options               | map       | 可选  | -         | tcp相关选项 |
| tcp_options.listen_ip     | string    | 可选  | "0.0.0.0" | tcp监听IP |
| tcp_options.listen_port   | int       | 必选  | -         | tcp监听端口，端口不能被占用 |
| udp_options               | map       | 可选  | -         | udp相关选项 |
| udp_options.listen_ip     | string    | 可选  | "0.0.0.0" | udp监听IP |
| udp_options.listen_port   | int       | 必选  | -         | udp监听端口，端口不能被占用 |
| udp_options.max_pkg_size  | int       | 可选  | 1024      | udp包最大尺寸，理论上最大不能超过65515 |


以下是一个简单的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: net_plugin # 【必选】插件名称
        path: ./libaimrt_net_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          thread_num: 4 # 【必选】net插件需要使用的线程数
          http_options: # 【可选】http相关选项
            listen_ip: 127.0.0.1 # 【可选】http监听IP
            listen_port: 50080 # 【必选】http监听端口，端口不能被占用
          tcp_options: # 【可选】tcp相关选项
            listen_ip: 127.0.0.1 # 【可选】tcp监听IP
            listen_port: 50081 # 【必选】tcp监听端口，端口不能被占用
          udp_options: # 【可选】udp相关选项
            listen_ip: 127.0.0.1 # 【可选】udp监听IP
            listen_port: 50082 # 【必选】udp监听端口，端口不能被占用
            max_pkg_size: 1024 # 【可选】udp包最大尺寸，理论上最大不能超过65515
```


关于**net_plugin**的配置，使用注意点如下：
- `thread_num`表示net插件需要使用的线程数。
- `http_options`是可选的，但只有当该节点被配置时，才能开启与http相关的功能，如http Channel后端、http RPC后端。
  - `http_options.listen_ip`用于配置http服务监听的地址，默认是"0.0.0.0"，如果仅想在指定网卡上监听，可以将其改为指定IP。
  - `http_options.listen_port`用于配置http服务监听的端口，此项为必填项，使用者必须确保端口未被占用，否则插件会初始化失败。
- `tcp_options`是可选的，但只有当该节点被配置时，才能开启与tcp相关的功能，如tcp Channel后端。
  - `tcp_options.listen_ip`用于配置tcp服务监听的地址，默认是"0.0.0.0"，如果仅想在指定网卡上监听，可以将其改为指定IP。
  - `tcp_options.listen_port`用于配置tcp服务监听的端口，此项为必填项，使用者必须确保端口未被占用，否则插件会初始化失败。
- `udp_options`是可选的，但只有当该节点被配置时，才能开启与udp相关的功能，如udp Channel后端。
  - `udp_options.listen_ip`用于配置udp服务监听的地址，默认是"0.0.0.0"，如果仅想在指定网卡上监听，可以将其改为指定IP。
  - `udp_options.listen_port`用于配置udp服务监听的端口，此项为必填项，使用者必须确保端口未被占用，否则插件会初始化失败。
  - `udp_options.max_pkg_size`用于配置udp包最大尺寸，理论上最大不能超过65515。发布的消息序列化之后必须小于这个值，否则会发布失败。


此外，在使用**net_plugin**时，Channel订阅回调、RPC Server处理方法、RPC Client返回时，使用的都是**net_plugin**提供的自有线程执行器，当使用者在回调中阻塞了线程时，有可能导致**net_plugin**线程池耗尽，从而无法继续接收/发送消息。正如Module接口文档中所述，一般来说，如果回调中的任务非常轻量，那就可以直接在回调里处理；但如果回调中的任务比较重，那最好调度到其他专门执行任务的执行器里处理。



## `http` RPC后端


`http`类型的RPC后端是**net_plugin**中提供的一种RPC后端，用于通过HTTP的方式来调用和处理RPC请求。其所有的配置项如下：


| 节点                          | 类型      | 是否可选| 默认值 | 作用 |
| ----                          | ----      | ----  | ----  | ---- |
| clients_options               | array     | 可选  | []    | 客户端发起RPC请求时的规则 |
| clients_options[i].func_name  | string    | 必选  | ""    | RPC Func名称，支持正则表达式 |
| clients_options[i].server_url | string    | 必选  | ""    | RPC Func发起调用时请求的url |

以下是一个简单的客户端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: net_plugin # 【必选】插件名称
        path: ./libaimrt_net_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          thread_num: 4 # 【必选】net插件需要使用的线程数
          http_options: # 【可选】http相关选项
            listen_ip: 127.0.0.1 # 【可选】http监听IP
            listen_port: 50081 # 【必选】http监听端口，端口不能被占用
  rpc: # 【可选】RPC配置根节点
    backends: # 【可选】RPC后端列表
      - type: http # 【必选】RPC后端类型
        options: # 【可选】RPC Client配置
          clients_options: # 【可选】客户端发起RPC请求时的规则
            - func_name: "(.*)" # 【必选】RPC Func名称，支持正则表达式
              server_url: http://127.0.0.1:50080 # 【必选】RPC Func发起调用时请求的url
    clients_options: # 【可选】RPC Client配置
      - func_name: "(.*)" # 【必选】RPC Client名称，支持正则表达式
        enable_backends: [http] # 【必选】RPC Client允许使用的RPC后端列表
```

以下则是一个简单的服务端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: net_plugin # 【必选】插件名称
        path: ./libaimrt_net_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          thread_num: 4 # 【必选】net插件需要使用的线程数
          http_options: # 【可选】http相关选项
            listen_ip: 127.0.0.1 # 【可选】http监听IP
            listen_port: 50080 # 【必选】http监听端口，端口不能被占用
  rpc: # 【可选】RPC配置根节点
    backends: # 【可选】RPC后端列表
      - type: http # 【必选】RPC后端类型
    servers_options: # 【可选】RPC Server配置
      - func_name: "(.*)" # 【必选】RPC Server名称，支持正则表达式
        enable_backends: [http] # 【必选】RPC Server允许使用的RPC后端列表
```

以上示例中，Server端监听了本地的50080端口，Client端则配置了所有的RPC请求都通过http后端请求到`http://127.0.0.1:50080`这个地址，也就是服务端监听的地址，从而完成RPC的调用闭环。


Client端向Server端发起调用时，遵循的格式如下：
- 使用HTTP POST方式发送，Body中填充消息序列化后的请求包数据或回包数据；
- 由`content-type` Header来定义Body内容的序列化方式，例如：
  - `content-type:application/json`
  - `content-type:application/protobuf`
- URL的编码方式：`http://{IP:PORT}/rpc/{FUNC_NAME}`：
  - `{IP:PORT}`：对端的网络地址；
  - `{MSG_TYPE}`：URL编码后的RPC func名称，以注册RPC时的TypeSupport类中定义的为准；



只要遵循这个格式，使用者可以基于PostMan或者Curl等工具发起调用，以JSON作为序列化方式来提高可读性，例如通过以下命令，即可发送一个消息到指定模块上去触发对应的回调：
```shell
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.example.ExampleService/GetFooData' \
    -d '{"msg": "test msg"}'
```

基于这个特性，`http`类型的RPC后端常用于调试开发或测试阶段，可以通过辅助工具快速触发某个Server处理函数，或通过抓包工具来检查上下游之间的通信内容。


## `http` Channel后端


`http`类型的Channel后端是**net_plugin**中提供的一种Channel后端，用于将消息通过HTTP的方式来发布和订阅消息。其所有的配置项如下：


| 节点                                  | 类型          | 是否可选| 默认值 | 作用 |
| ----                                  | ----          | ----  | ----  | ---- |
| pub_topics_options                    | array         | 可选  | []    | 发布Topic时的规则 |
| pub_topics_options[i].topic_name      | string        | 必选  | ""    | Topic名称，支持正则表达式 |
| pub_topics_options[i].server_url_list | string array  | 必选  | []    | Topic需要被发送的url列表 |

以下是一个简单的发布端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: net_plugin # 【必选】插件名称
        path: ./libaimrt_net_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          thread_num: 4 # 【必选】net插件需要使用的线程数
          http_options: # 【可选】http相关选项
            listen_ip: 127.0.0.1 # 【可选】http监听IP
            listen_port: 50081 # 【必选】http监听端口，端口不能被占用
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: http # 【必选】Channel后端类型
        options: # 【可选】具体Channel后端的配置
          pub_topics_options: # 【可选】发布Topic时的规则
            - topic_name: "(.*)" # 【必选】Topic名称，支持正则表达式
              server_url_list: ["127.0.0.1:50080"] # 【必选】Topic需要被发送的url列表
    pub_topics_options: # 【可选】Channel Pub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Pub Topic名称，支持正则表达式
        enable_backends: [http] # 【必选】Channel Pub Topic允许使用的Channel后端列表
```

以下则是一个简单的订阅端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: net_plugin # 【必选】插件名称
        path: ./libaimrt_net_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          thread_num: 4 # 【必选】net插件需要使用的线程数
          http_options: # 【可选】http相关选项
            listen_ip: 127.0.0.1 # 【可选】http监听IP
            listen_port: 50080 # 【必选】http监听端口，端口不能被占用
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: http # 【必选】Channel后端类型
    sub_topics_options: # 【可选】Channel Sub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Sub Topic名称，支持正则表达式
        enable_backends: [http] # 【必选】Channel Sub Topic允许使用的Channel后端列表
```

以上示例中，订阅端监听了本地的50080端口，发布端则配置了所有的Topic消息都通过http后端请求到`127.0.0.1:50080`这个地址，也就是服务端监听的地址，订阅端也配置了所有消息都可以从http后端触发回调，从而打通消息发布订阅的链路。

发布端向订阅端发布消息时，遵循的格式如下：
- 使用HTTP POST方式发送，Body中填充消息序列化后的数据；
- 由`content-type` Header来定义Body内容的序列化方式，例如：
  - `content-type:application/json`
  - `content-type:application/protobuf`
- URL的编码方式：`http://{IP:PORT}/channel/{TOPIC_NAME}/{MSG_TYPE}`：
  - `{IP:PORT}`：对端的网络地址；
  - `{TOPIC_NAME}`：URL编码后的Topic名称；
  - `{MSG_TYPE}`：URL编码后的消息类型名称，消息类型名称以TypeSupport类中定义的为准；


只要遵循这个格式，使用者可以基于PostMan或者Curl等工具发起调用，以JSON作为序列化方式来提高可读性，例如通过以下命令，即可发送一个消息到指定模块上去触发对应的回调：
```shell
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/channel/test_topic/pb%3Aaimrt.protocols.example.ExampleEventMsg' \
    -d '{"msg": "test msg", "num": 123}'
```

基于这个特性，`http`类型的Channel后端常用于调试开发或测试阶段，可以通过辅助工具快速触发某个回调，或通过抓包工具来检查上下游之间的通信内容。



## `tcp` Channel后端


`tcp`类型的Channel后端是**net_plugin**中提供的一种Channel后端，用于将消息通过tcp的方式来发布和订阅消息。其所有的配置项如下：


| 节点                                  | 类型          | 是否可选| 默认值 | 作用 |
| ----                                  | ----          | ----  | ----  | ---- |
| pub_topics_options                    | array         | 可选  | []    | 发布Topic时的规则 |
| pub_topics_options[i].topic_name      | string        | 必选  | ""    | Topic名称，支持正则表达式 |
| pub_topics_options[i].server_url_list | string array  | 必选  | []    | Topic需要被发送的url列表 |

以下是一个简单的发布端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: net_plugin # 【必选】插件名称
        path: ./libaimrt_net_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          thread_num: 4 # 【必选】net插件需要使用的线程数
          tcp_options: # 【可选】tcp相关选项
            listen_ip: 127.0.0.1 # 【可选】tcp监听IP
            listen_port: 50081 # 【必选】tcp监听端口，端口不能被占用
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: tcp # 【必选】Channel后端类型
        options: # 【可选】具体Channel后端的配置
          pub_topics_options: # 【可选】发布Topic时的规则
            - topic_name: "(.*)" # 【必选】Topic名称，支持正则表达式
              server_url_list: ["127.0.0.1:50080"] # 【必选】Topic需要被发送的url列表
    pub_topics_options: # 【可选】Channel Pub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Pub Topic名称，支持正则表达式
        enable_backends: [tcp] # 【必选】Channel Pub Topic允许使用的Channel后端列表
```

以下则是一个简单的订阅端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: net_plugin # 【必选】插件名称
        path: ./libaimrt_net_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          thread_num: 4 # 【必选】net插件需要使用的线程数
          tcp_options: # 【可选】tcp相关选项
            listen_ip: 127.0.0.1 # 【可选】tcp监听IP
            listen_port: 50080 # 【必选】tcp监听端口，端口不能被占用
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: tcp # 【必选】Channel后端类型
    sub_topics_options: # 【可选】Channel Sub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Sub Topic名称，支持正则表达式
        enable_backends: [tcp] # 【必选】Channel Sub Topic允许使用的Channel后端列表
```

以上示例中，订阅端监听了本地的50080端口，发布端则配置了所有的Topic消息都通过tcp后端请求到`127.0.0.1:50080`这个地址，也就是服务端监听的地址，从而打通消息发布订阅的链路。

注意，使用tcp后端传输消息时，数据格式是私有的，随着版本的更新，数据格式可能会变化。



## `udp` Channel后端


`udp`类型的Channel后端是**net_plugin**中提供的一种Channel后端，用于将消息通过udp的方式来发布和订阅消息。其所有的配置项如下：


| 节点                                  | 类型          | 是否可选| 默认值 | 作用 |
| ----                                  | ----          | ----  | ----  | ---- |
| pub_topics_options                    | array         | 可选  | []    | 发布Topic时的规则 |
| pub_topics_options[i].topic_name      | string        | 必选  | ""    | Topic名称，支持正则表达式 |
| pub_topics_options[i].server_url_list | string array  | 必选  | []    | Topic需要被发送的url列表 |

以下是一个简单的发布端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: net_plugin # 【必选】插件名称
        path: ./libaimrt_net_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          thread_num: 4 # 【必选】net插件需要使用的线程数
          udp_options: # 【可选】udp相关选项
            listen_ip: 127.0.0.1 # 【可选】udp监听IP
            listen_port: 50081 # 【必选】udp监听端口，端口不能被占用
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: udp # 【必选】Channel后端类型
        options: # 【可选】具体Channel后端的配置
          pub_topics_options: # 【可选】发布Topic时的规则
            - topic_name: "(.*)" # 【必选】Topic名称，支持正则表达式
              server_url_list: ["127.0.0.1:50080"] # 【必选】Topic需要被发送的url列表
    pub_topics_options: # 【可选】Channel Pub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Pub Topic名称，支持正则表达式
        enable_backends: [udp] # 【必选】Channel Pub Topic允许使用的Channel后端列表
```

以下则是一个简单的订阅端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: net_plugin # 【必选】插件名称
        path: ./libaimrt_net_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          thread_num: 4 # 【必选】net插件需要使用的线程数
          udp_options: # 【可选】udp相关选项
            listen_ip: 127.0.0.1 # 【可选】udp监听IP
            listen_port: 50080 # 【必选】udp监听端口，端口不能被占用
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: udp # 【必选】Channel后端类型
    sub_topics_options: # 【可选】Channel Sub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Sub Topic名称，支持正则表达式
        enable_backends: [udp] # 【必选】Channel Sub Topic允许使用的Channel后端列表
```

以上示例中，订阅端监听了本地的50080端口，发布端则配置了所有的Topic消息都通过udp后端请求到`127.0.0.1:50080`这个地址，也就是服务端监听的地址，从而打通消息发布订阅的链路。

注意，使用udp后端传输消息时，数据格式是私有的，随着版本的更新，数据格式可能会变化。
