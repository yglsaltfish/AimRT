# ros2插件



## 插件概述


**ros2_plugin**是一个基于ROS2 Humble版本实现的通信传输插件，此插件为AimRT提供了以下组件：
- `ros2`类型RPC后端
- `ros2`类型Channel后端

插件的配置项如下：

| 节点                      | 类型      | 是否可选| 默认值          | 作用 |
| ----                      | ----      | ----  | ----              | ---- |
| node_name                 | string    | 必选  | ""                | ROS2节点名称 |
| executor_type             | string    | 可选  | "MultiThreaded"   | ROS2执行器类型，可选值："SingleThreaded"、"StaticSingleThreaded"、"MultiThreaded"|
| executor_thread_num       | int       | 可选  | 2                 | 当 executor_type == "MultiThreaded"时，表示ROS2执行器的线程数 |



以下是一个简单的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: ros2_plugin # 【必选】插件名称
        path: ./libaimrt_ros2_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          node_name: example_ros2_node # 【必选】ROS2节点名称
          executor_type: MultiThreaded # 【可选】ROS2执行器类型，可选值："SingleThreaded"、"StaticSingleThreaded"、"MultiThreaded"
          executor_thread_num: 4 # 【可选】当 executor_type == "MultiThreaded"时，表示ROS2执行器的线程数
```

关于**ros2_plugin**的配置，使用注意点如下：
- `node_name`表示ROS2节点名称，在外界看来，加载了ROS2插件的AimRT节点就是一个ROS2节点，它的node名称就是根据此项来配置。
- `executor_type`表示ROS2节点执行器的类型，当前有三种选择：`SingleThreaded`、`StaticSingleThreaded`、`MultiThreaded`，具体的含义请参考ROS2的文档。
- `executor_thread_num`仅在`executor_type`值为`MultiThreaded`时生效，表示ROS2的线程数。


此外，在使用**ros2_plugin**时，Channel订阅回调、RPC Server处理方法、RPC Client返回时，使用的都是**ros2_plugin**提供的执行器，当使用者在回调中阻塞了线程时，有可能导致**ros2_plugin**线程池耗尽，从而无法继续接收/发送消息。正如Module接口文档中所述，一般来说，如果回调中的任务非常轻量，那就可以直接在回调里处理；但如果回调中的任务比较重，那最好调度到其他专门执行任务的执行器里处理。


## `ros2` RPC后端


`ros2`类型的RPC后端是**ros2_plugin**中提供的一种RPC后端，用于通过ROS2 RPC的方式来调用和处理AimRT RPC请求。其所有的配置项如下：


| 节点                                              | 类型    | 是否可选 | 默认值     | 作用  |
| ----                                              | ----    | ----  | ----        | ----  |
| timeout_executor                                  | string  | 可选   | ""         | Client端RPC超时情况下的执行器                          |
| clients_options                                   | array   | 可选   | []         | 客户端发起RPC请求时的规则 |
| clients_options[i].func_name                      | string  | 必选   | ""         | RPC Func名称，支持正则表达式  |
| clients_options[i].qos                            | map     | 可选   | -          | QOS配置  |
| clients_options[i].qos.history                    | string  | 可选   | "default"  | QOS的历史记录选项<br/>keep_last:保留最近的记录(缓存最多N条记录，可通过队列长度选项来配置)<br/>keep_all:保留所有记录(缓存所有记录，但受限于底层中间件可配置的最大资源)<br/>default:使用系统默认   |
| clients_options[i].qos.depth                      | int     | 可选   | 10         | QOS的队列深度选项(只能与Keep_last配合使用)  |
| clients_options[i].qos.reliability                | string  | 可选   | "default"  | QOS的可靠性选项<br/>reliable:可靠的(消息丢失时，会重新发送,反复重传以保证数据传输成功)<br/>best_effort:尽力而为的(尝试传输数据但不保证成功传输,当网络不稳定时可能丢失数据)<br/>default:系统默认 |
| clients_options[i].qos.durability                 | string  | 可选   | "default"  | QOS的持续性选项<br/>transient_local:局部瞬态(发布器为晚连接(late-joining)的订阅器保留数据)<br/>volatile:易变态(不保留任何数据)<br/>default:系统默认 |
| clients_options[i].qos.deadline                   | int     | 可选   | -1         | QOS的后续消息发布到主题之间的预期最大时间量选项<br/>需填毫秒级时间间隔，填-1为不设置，按照系统默认  |
| clients_options[i].qos.lifespan                   | int     | 可选   | -1         | QOS的消息发布和接收之间的最大时间量(单位毫秒)选项<br/>而不将消息视为陈旧或过期（过期的消息被静默地丢弃，并且实际上从未被接收<br/>填-1保持系统默认 不设置  |
| clients_options[i].qos.liveliness                 | string  | 可选   | "default"  | QOS的如何确定发布者是否活跃选项<br/>automatic:自动(ROS2会根据消息发布和接收的时间间隔来判断)<br/>manual_by_topic:需要发布者定期声明<br/>default:保持系统默认  |
| clients_options[i].qos.liveliness_lease_duration  | int     | 可选   | -1         | QOS的活跃性租期的时长(单位毫秒)选项，如果超过这个时间发布者没有声明活跃，则被认为是不活跃的<br/>填-1保持系统默认 不设置 |
| servers_options                                   | array   | 可选   | []         | 服务端接收处理RPC请求时的规则 |
| servers_options[i].func_name                      | string  | 必选   | ""         | RPC Func名称，支持正则表达式  |
| servers_options[i].qos                            | map     | 可选   | -          | QOS配置  |
| servers_options[i].qos.history                    | string  | 可选   | "default"  | QOS的历史记录选项<br/>keep_last:保留最近的记录(缓存最多N条记录，可通过队列长度选项来配置)<br/>keep_all:保留所有记录(缓存所有记录，但受限于底层中间件可配置的最大资源)<br/>default:使用系统默认   |
| servers_options[i].qos.depth                      | int     | 可选   | 10         | QOS的队列深度选项(只能与Keep_last配合使用)  |
| servers_options[i].qos.reliability                | string  | 可选   | "default"  | QOS的可靠性选项<br/>reliable:可靠的(消息丢失时，会重新发送,反复重传以保证数据传输成功)<br/>best_effort:尽力而为的(尝试传输数据但不保证成功传输,当网络不稳定时可能丢失数据)<br/>default:系统默认 |
| servers_options[i].qos.durability                 | string  | 可选   | "default"  | QOS的持续性选项<br/>transient_local:局部瞬态(发布器为晚连接(late-joining)的订阅器保留数据)<br/>volatile:易变态(不保留任何数据)<br/>default:系统默认 |
| servers_options[i].qos.deadline                   | int     | 可选   | -1         | QOS的后续消息发布到主题之间的预期最大时间量选项<br/>需填毫秒级时间间隔，填-1为不设置，按照系统默认  |
| servers_options[i].qos.lifespan                   | int     | 可选   | -1         | QOS的消息发布和接收之间的最大时间量(单位毫秒)选项<br/>而不将消息视为陈旧或过期（过期的消息被静默地丢弃，并且实际上从未被接收<br/>填-1保持系统默认 不设置  |
| servers_options[i].qos.liveliness                 | string  | 可选   | "default"  | QOS的如何确定发布者是否活跃选项<br/>automatic:自动(ROS2会根据消息发布和接收的时间间隔来判断)<br/>manual_by_topic:需要发布者定期声明<br/>default:保持系统默认  |
| servers_options[i].qos.liveliness_lease_duration  | int     | 可选   | -1         | QOS的活跃性租期的时长(单位毫秒)选项，如果超过这个时间发布者没有声明活跃，则被认为是不活跃的<br/>填-1保持系统默认 不设置 |



以下是一个简单的客户端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: ros2_plugin # 【必选】插件名称
        path: ./libaimrt_ros2_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          node_name: example_ros2_client_node # 【必选】ROS2节点名称
          executor_type: MultiThreaded # 【可选】ROS2执行器类型，可选值："SingleThreaded"、"StaticSingleThreaded"、"MultiThreaded"
          executor_thread_num: 4 # 【可选】当 executor_type == "MultiThreaded"时，表示ROS2执行器的线程数
  rpc: # 【可选】RPC配置根节点
    backends: # 【可选】RPC后端列表
      - type: ros2 # 【必选】RPC后端类型
        options:
          clients_options:
            - func_name: "(.*)" # 【必选】此配置匹配的RPC Client名称，支持正则表达式
              qos:
                history: keep_last #keep_last:保存最后的,keep_all:保存所有的记录,default:保持系统默认
                depth: 10  #队列深度 与keep_last搭配使用
                reliability: reliable #reliable:可靠的,best_effort:尽力而为的,default:保持系统默认
                durability: volatile #持续性选项,volatile:易变态(不保留任何数据),transient_local:局部瞬态(发布器为晚连接(late-joining)的订阅器保留数据),default:保持系统默认
                deadline: -1 #后续消息发布到主题之间的预期最大时间量(单位毫秒) -1保持系统默认 不设置
                lifespan: -1 #消息发布和接收之间的最大时间量(单位毫秒)，而不将消息视为陈旧或过期（过期的消息被静默地丢弃，并且实际上从未被接收）  -1保持系统默认 不设置
                liveliness: automatic #如何确定发布者是否活跃,automatic:自动(ROS2会根据消息发布和接收的时间间隔来判断) manual_by_topic:需要发布者定期声明,default:保持系统默认
                liveliness_lease_duration: -1 #活跃性租期的时长(单位毫秒)，如果超过这个时间发布者没有声明活跃，则被认为是不活跃的。 -1保持系统默认 不设置
    clients_options: # 【可选】RPC Client配置
      - func_name: "(.*)" # 【必选】RPC Client名称，支持正则表达式
        enable_backends: [ros2] # 【必选】RPC Client允许使用的RPC后端列表
```

以下则是一个简单的服务端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: ros2_plugin # 【必选】插件名称
        path: ./libaimrt_ros2_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          node_name: example_ros2_server_node # 【必选】ROS2节点名称
          executor_type: MultiThreaded # 【可选】ROS2执行器类型，可选值："SingleThreaded"、"StaticSingleThreaded"、"MultiThreaded"
          executor_thread_num: 4 # 【可选】当 executor_type == "MultiThreaded"时，表示ROS2执行器的线程数
  rpc: # 【可选】RPC配置根节点
    backends: # 【可选】RPC后端列表
      - type: ros2 # 【必选】RPC后端类型
        options:
          servers_options:
            - func_name: "(.*)" # 【必选】此配置匹配的RPC Server名称，支持正则表达式
              qos:
                history: keep_last #keep_last:保存最后的,keep_all:保存所有的记录,default:保持系统默认
                depth: 10  #队列深度 与keep_last搭配使用
                reliability: reliable #reliable:可靠的,best_effort:尽力而为的,default:保持系统默认
                durability: volatile #持续性选项,volatile:易变态(不保留任何数据),transient_local:局部瞬态(发布器为晚连接(late-joining)的订阅器保留数据),default:保持系统默认
                deadline: -1 #后续消息发布到主题之间的预期最大时间量(单位毫秒) -1保持系统默认 不设置
                lifespan: -1 #消息发布和接收之间的最大时间量(单位毫秒)，而不将消息视为陈旧或过期（过期的消息被静默地丢弃，并且实际上从未被接收）  -1保持系统默认 不设置
                liveliness: automatic #如何确定发布者是否活跃,automatic:自动(ROS2会根据消息发布和接收的时间间隔来判断) manual_by_topic:需要发布者定期声明,default:保持系统默认
                liveliness_lease_duration: -1 #活跃性租期的时长(单位毫秒)，如果超过这个时间发布者没有声明活跃，则被认为是不活跃的。 -1保持系统默认 不设置
    servers_options: # 【可选】RPC Server配置
      - func_name: "(.*)" # 【必选】RPC Server名称，支持正则表达式
        enable_backends: [ros2] # 【必选】RPC Server允许使用的RPC后端列表
```

以上示例中，Server端启动了一个ROS2节点`example_ros2_server_node`，Client端则启动了一个ROS2节点`example_ros2_client_node`，Client端通过ROS2的后端发起RPC调用，Server端通过ROS2后端接收到RPC请求并进行处理。

Client端向Server端发起调用时，如果协议层是原生ROS2协议，那么通信时将完全复用ROS2的原生协议，原生ROS2节点可以基于该协议无缝与AimRT节点对接。

如果Client端向Server端发起调用时，协议层没有使用ROS2协议，那么通信时将基于{{ '[RosRpcWrapper.srv]({}/src/protocols/plugins/ros2_plugin_proto/srv/RosRpcWrapper.srv)'.format(code_site_root_path_url) }}这个ROS2协议进行包装，该协议内容如下：
```
string  serialization_type
byte[]  data
---
int64   code
string  serialization_type
byte[]  data
```

如果此时原生的ROS2节点需要和AimRT的节点对接，原生ROS2节点的开发者需要搭配此协议Req/Rsp的data字段来序列化/反序列化真正的请求包/回包。

此外，由于ROS2对`service_name`有一些特殊要求，AimRT与ROS2互通时的`service_name`由AimRT Func根据一个类似于URL编码的规则来生成：将所有除数字、字母和'/'的符号的ascii码以HEX编码，加上'_'作为前缀。例如，如果AimRT Func名称为`/aaa.bbb.ccc/ddd`，则编码后的ROS2 service name就是`/aaa_2Ebbb_2Eccc/ddd`。具体值也会在ros2_plugin启动时打印出来。

基于以上特性，`ros2`类型的RPC后端可以用于打通与原生ROS2节点的RPC链路，从而实现AimRT对ROS2的兼容。

## `ros2` Channel后端


`ros2`类型的Channel后端是**ros2_plugin**中提供的一种Channel后端，用于通过ROS2 Topic的方式来发布和订阅AimRT Channel消息。其所有的配置如下:

| 节点                                                  | 类型  | 是否可选| 默认值 | 作用 |
| ----                                                | ----    | ----  | ----  | ---- |
| pub_topics_options                                  | array   | 可选  | []        | 发布Topic时的规则 |
| pub_topics_options[i].topic_name                    | string  | 必选  | ""        | Topic名称，支持正则表达式 |
| pub_topics_options[i].qos                           | map     | 可选  | -         | QOS配置  |
| pub_topics_options[i].qos.history                   | string  | 可选  | "default" | QOS的历史记录选项<br/>keep_last:保留最近的记录(缓存最多N条记录，可通过队列长度选项来配置)<br/>keep_all:保留所有记录(缓存所有记录，但受限于底层中间件可配置的最大资源)<br/>default:使用系统默认   |
| pub_topics_options[i].qos.depth                     | int     | 可选  | 10        | QOS的队列深度选项(只能与Keep_last配合使用)  |
| pub_topics_options[i].qos.reliability               | string  | 可选  | "default" | QOS的可靠性选项<br/>reliable:可靠的(消息丢失时，会重新发送,反复重传以保证数据传输成功)<br/>best_effort:尽力而为的(尝试传输数据但不保证成功传输,当网络不稳定时可能丢失数据)<br/>default:系统默认 |
| pub_topics_options[i].qos.durability                | string  | 可选  | "default" | QOS的持续性选项<br/>transient_local:局部瞬态(发布器为晚连接(late-joining)的订阅器保留数据)<br/>volatile:易变态(不保留任何数据)<br/>default:系统默认               |
| pub_topics_options[i].qos.deadline                  | int     | 可选  | -1        | QOS的后续消息发布到主题之间的预期最大时间量选项<br/>需填毫秒级时间间隔，填-1为不设置，按照系统默认                                                                     |
| pub_topics_options[i].qos.lifespan                  | int     | 可选  | -1        | QOS的消息发布和接收之间的最大时间量(单位毫秒)选项<br/>而不将消息视为陈旧或过期（过期的消息被静默地丢弃，并且实际上从未被接收<br/>填-1保持系统默认 不设置                                      |
| pub_topics_options[i].qos.liveliness                | string  | 可选  | "default" | QOS的如何确定发布者是否活跃选项<br/>automatic:自动(ROS2会根据消息发布和接收的时间间隔来判断)<br/>manual_by_topic:需要发布者定期声明<br/>default:保持系统默认                |
| pub_topics_options[i].qos.liveliness_lease_duration | int     | 可选  | -1        | QOS的活跃性租期的时长(单位毫秒)选项，如果超过这个时间发布者没有声明活跃，则被认为是不活跃的<br/>填-1保持系统默认 不设置                                                         |
| sub_topics_options                                  | array   | 可选  | []        | 订阅Topic时的规则 |
| sub_topics_options[i].topic_name                    | string  | 必选  | ""        | Topic名称，支持正则表达式 |
| sub_topics_options[i].qos                           | map     | 可选  | -         | QOS配置  |
| sub_topics_options[i].qos.history                   | string  | 可选  | "default" | QOS的历史记录选项<br/>keep_last:保留最近的记录(缓存最多N条记录，可通过队列长度选项来配置)<br/>keep_all:保留所有记录(缓存所有记录，但受限于底层中间件可配置的最大资源)<br/>default:使用系统默认   |
| sub_topics_options[i].qos.depth                     | int     | 可选  | 10        | QOS的队列深度选项(只能与Keep_last配合使用)  |
| sub_topics_options[i].qos.reliability               | string  | 可选  | "default" | QOS的可靠性选项<br/>reliable:可靠的(消息丢失时，会重新发送,反复重传以保证数据传输成功)<br/>best_effort:尽力而为的(尝试传输数据但不保证成功传输,当网络不稳定时可能丢失数据)<br/>default:系统默认 |
| sub_topics_options[i].qos.durability                | string  | 可选  | "default" | QOS的持续性选项<br/>transient_local:局部瞬态(发布器为晚连接(late-joining)的订阅器保留数据)<br/>volatile:易变态(不保留任何数据)<br/>default:系统默认               |
| sub_topics_options[i].qos.deadline                  | int     | 可选  | -1        | QOS的后续消息发布到主题之间的预期最大时间量选项<br/>需填毫秒级时间间隔，填-1为不设置，按照系统默认                                                                     |
| sub_topics_options[i].qos.lifespan                  | int     | 可选  | -1        | QOS的消息发布和接收之间的最大时间量(单位毫秒)选项<br/>而不将消息视为陈旧或过期（过期的消息被静默地丢弃，并且实际上从未被接收<br/>填-1保持系统默认 不设置                                      |
| sub_topics_options[i].qos.liveliness                | string  | 可选  | "default" | QOS的如何确定发布者是否活跃选项<br/>automatic:自动(ROS2会根据消息发布和接收的时间间隔来判断)<br/>manual_by_topic:需要发布者定期声明<br/>default:保持系统默认                |
| sub_topics_options[i].qos.liveliness_lease_duration | int     | 可选  | -1        | QOS的活跃性租期的时长(单位毫秒)选项，如果超过这个时间发布者没有声明活跃，则被认为是不活跃的<br/>填-1保持系统默认 不设置                                                         |


以下是一个简单的发布端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: ros2_plugin # 【必选】插件名称
        path: ./libaimrt_ros2_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          node_name: example_ros2_pub_node # 【必选】ROS2节点名称
          executor_type: MultiThreaded # 【可选】ROS2执行器类型，可选值："SingleThreaded"、"StaticSingleThreaded"、"MultiThreaded"
          executor_thread_num: 4 # 【可选】当 executor_type == "MultiThreaded"时，表示ROS2执行器的线程数
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: ros2 # 【必选】Channel后端类型
        options:
          pub_topics_options:
            - topic_name: "(.*)" # 【必选】此配置匹配的topic名称，支持正则表达式
              qos:
                history: keep_last #keep_last:保存最后的,keep_all:保存所有的记录,default:保持系统默认
                depth: 10  #队列深度 与keep_last搭配使用
                reliability: reliable #reliable:可靠的,best_effort:尽力而为的,default:保持系统默认
                durability: volatile #持续性选项,volatile:易变态(不保留任何数据),transient_local:局部瞬态(发布器为晚连接(late-joining)的订阅器保留数据),default:保持系统默认
                deadline: -1 #后续消息发布到主题之间的预期最大时间量(单位毫秒) -1保持系统默认 不设置
                lifespan: -1 #消息发布和接收之间的最大时间量(单位毫秒)，而不将消息视为陈旧或过期（过期的消息被静默地丢弃，并且实际上从未被接收）  -1保持系统默认 不设置
                liveliness: automatic #如何确定发布者是否活跃,automatic:自动(ROS2会根据消息发布和接收的时间间隔来判断) manual_by_topic:需要发布者定期声明,default:保持系统默认
                liveliness_lease_duration: -1 #活跃性租期的时长(单位毫秒)，如果超过这个时间发布者没有声明活跃，则被认为是不活跃的。 -1保持系统默认 不设置
    pub_topics_options: # 【可选】Channel Pub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Pub Topic名称，支持正则表达式
        enable_backends: [ros2] # 【必选】Channel Pub Topic允许使用的Channel后端列表
```

以下则是一个简单的订阅端的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: ros2_plugin # 【必选】插件名称
        path: ./libaimrt_ros2_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          node_name: example_ros2_sub_node # 【必选】ROS2节点名称
          executor_type: MultiThreaded # 【可选】ROS2执行器类型，可选值："SingleThreaded"、"StaticSingleThreaded"、"MultiThreaded"
          executor_thread_num: 4 # 【可选】当 executor_type == "MultiThreaded"时，表示ROS2执行器的线程数
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: ros2 # 【必选】Channel后端类型
        options:
          sub_topics_options:
            - topic_name: "(.*)" # 【必选】此配置匹配的topic名称，支持正则表达式
              qos:
                history: keep_last #keep_last:保存最后的,keep_all:保存所有的记录,default:保持系统默认
                depth: 10  #队列深度 与keep_last搭配使用
                reliability: reliable #reliable:可靠的,best_effort:尽力而为的,default:保持系统默认
                durability: volatile #持续性选项,volatile:易变态(不保留任何数据),transient_local:局部瞬态(发布器为晚连接(late-joining)的订阅器保留数据),default:保持系统默认
                deadline: -1 #后续消息发布到主题之间的预期最大时间量(单位毫秒) -1保持系统默认 不设置
                lifespan: -1 #消息发布和接收之间的最大时间量(单位毫秒)，而不将消息视为陈旧或过期（过期的消息被静默地丢弃，并且实际上从未被接收）  -1保持系统默认 不设置
                liveliness: automatic #如何确定发布者是否活跃,automatic:自动(ROS2会根据消息发布和接收的时间间隔来判断) manual_by_topic:需要发布者定期声明,default:保持系统默认
                liveliness_lease_duration: -1 #活跃性租期的时长(单位毫秒)，如果超过这个时间发布者没有声明活跃，则被认为是不活跃的。 -1保持系统默认 不设置
    sub_topics_options: # 【可选】Channel Sub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Sub Topic名称，支持正则表达式
        enable_backends: [ros2] # 【必选】Channel Sub Topic允许使用的Channel后端列表
```

以上示例中，发布端启动了一个ROS2节点`example_ros2_pub_node`，订阅端则启动了一个ROS2节点`example_ros2_sub_node`，发布端通过ROS2的后端发布消息，订阅端通过ROS2后端接收到消息并进行处理。

如果消息发布订阅时，协议层是原生ROS2协议，那么通信时将完全复用ROS2的原生协议，原生ROS2节点可以基于该协议无缝与AimRT节点对接。

如果消息发布订阅时，协议层没有使用ROS2协议，那么通信时将基于{{ '[RosMsgWrapper.msg]({}/src/protocols/plugins/ros2_plugin_proto/msg/RosMsgWrapper.msg)'.format(code_site_root_path_url) }}这个ROS2协议进行包装，该协议内容如下：
```
string  serialization_type
byte[]  data
```

如果此时原生的ROS2节点需要和AimRT的节点对接，原生ROS2节点的开发者需要搭配此协议的data字段来序列化/反序列化真正的消息。

此外，AimRT与ROS2互通时的`Topic`名称由以下规则生成：`${aimrt_topic}/${ros2_encode_aimrt_msg_type}`。其中`${aimrt_topic}`是AimRT Topic名称，`${ros2_encode_aimrt_msg_type}`根据AimRT Msg名称由一个类似于URL编码的规则来生成：将所有除数字、字母和'/'的符号的ascii码以HEX编码，加上'_'作为前缀。例如，如果AimRT Topic名称是`test_topic`，AimRT Msg名称为`pb:aaa.bbb.ccc`，则最终ROS2 Topic值就是`test_topic/pb_3Aaaa_2Ebbb_2Eccc`。具体值也会在ros2_plugin启动时打印出来。

基于这个特性，`ros2`类型的Channel后端可以用于打通与原生ROS2节点的Channel链路，从而实现AimRT对ROS2的兼容。
