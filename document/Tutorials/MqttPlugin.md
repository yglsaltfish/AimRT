# Mqtt插件


## 协议结构

### channel

#### Mqtt Topic名称格式
```
/channel/${topic_name}/${message_type}
```
其中，${topic_name}为aimrt的topic名称，message_type为url编码后的aimrt消息名称。

例如，aimrt topic名称为`test_topic`，消息类型为`pb:aimrt.protocols.example.ExampleEventMsg`，则最终Mqtt的topic名称为：

```
/channel/test_topic/pb%3Aaimrt.protocols.example.ExampleEventMsg
```


#### Mqtt数据包格式
整体分两段：
- 序列化类型，一般是`pb`或`json`
- 数据

```
| n(0~255) [1 byte] | content type [n byte] | msg data [len - 1 - n byte] |
```

### rpc

#### Mqtt Topic名称格式
- Server端
  - 订阅请求包使用的topic：`$share/aimrt/aimrt_rpc_req/${func_name}`
  - 发布回包使用的topic：`aimrt_rpc_rsp/${client_id}/${func_name}`
- Client端
  - 发布请求包使用的topic：`aimrt_rpc_rsp/${client_id}/${func_name}`
  - 订阅回包使用的topic：`aimrt_rpc_req/${func_name}`

其中`${client_id}`是Client端需要保证在同一个Mqtt broker环境下全局唯一的一个值，一般使用在Mqtt broker处注册的client_id。`${func_name}`是url编码后的aimrt rpc方法名称。Server端订阅使用共享订阅，保证只有一个服务端处理请求。此项特性需要支持Mqtt5.0协议的Broker。

例如，client端向Mqtt broker注册的id为`example_client`，func名称为`/aimrt.protocols.example.ExampleService/GetBarData`，则`${client_id}`值为`example_client`，`${func_name}`值为`%2Faimrt.protocols.example.ExampleService%2FGetBarData`。


#### Mqtt数据包格式

Client -> Server，整体分4段:
- 序列化类型，一般是`pb`或`json`
- client端想要server端回复rsp的mqtt topic名称。client端自己需要订阅这个mqtt topic
- msg id，4字节，server端会原封不动的封装到rsp包里，供client端定位rsp对应哪个req
- 数据

```
| n(0~255) [1 byte] | content type [n byte] 
| m(0~255) [1 byte] | rsp topic name [m byte] 
| msg id [4 byte] 
| msg data [len - 1 - n - 1 - m - 4 byte] |
```

Server -> Client，整体分3段:
- 序列化类型，一般是`pb`或`json`
- msg id，4字节，req中的msg id
- 数据

```
| n(0~255) [1 byte] | content type [n byte] 
| msg id [4 byte] 
| msg data [len - 1 - n - 4 byte] |
```
