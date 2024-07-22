# aimrt.rpc


## RPC配置概述

`aimrt.rpc`配置项用于配置RPC功能。其中的细节配置项说明如下：


| 节点                                | 类型      | 是否可选| 默认值 | 作用 |
| ----                                | ----      | ----  | ----  | ---- |
| backends                            | array     | 可选  | []    | RPC后端列表 |
| backends[i].type                    | string    | 必选  | ""    | RPC后端类型 |
| backends[i].options                 | map       | 可选  | -     | 具体RPC后端的配置 |
| clients_options                     | array     | 可选  | ""    | RPC Client配置 |
| clients_options[i].func_name        | string    | 必选  | ""    | RPC Client名称，支持正则表达式 |
| clients_options[i].enable_backends  | string array | 必选  | [] | RPC Client允许使用的RPC后端列表 |
| servers_options                     | array     | 可选  | ""    | RPC Server配置 |
| servers_options[i].func_name        | string    | 必选  | ""    | RPC Server名称，支持正则表达式 |
| servers_options[i].enable_backends  | string array | 必选  | [] | RPC Server允许使用的RPC后端列表 |
| client_filters                      | string array | 可选  | [] | RPC client端需要加载的框架侧过滤器列表 |
| server_filters                      | string array | 可选  | [] | RPC Server端需要加载的框架侧过滤器列表 |

以下是一个简单的示例：
```yaml
aimrt:
  rpc: # 【可选】RPC配置根节点
    backends: # 【可选】RPC后端列表
      - type: local # 【必选】RPC后端类型
      - type: mqtt # 【必选】RPC后端类型
    clients_options: # 【可选】RPC Client配置
      - func_name: "(.*)" # 【必选】RPC Client名称，支持正则表达式
        enable_backends: [local] # 【必选】RPC Client允许使用的RPC后端列表
    servers_options: # 【可选】RPC Server配置
      - func_name: "(.*)" # 【必选】RPC Server名称，支持正则表达式
        enable_backends: [local] # 【必选】RPC Server允许使用的RPC后端列表
    client_filters: [] # 【可选】RPC client端需要注册的框架侧过滤器列表
    server_filters: [] # 【可选】RPC Server端需要注册的框架侧过滤器列表
```



`aimrt.rpc`的配置说明如下：
- `backends`是一个数组，用于配置各个Rpc后端。
  - `backends[i].type`是Rpc后端的类型。AimRT官方提供了`local`后端，部分插件也提供了一些Rpc后端类型。
  - `backends[i].options`是AimRT传递给各个Rpc后端的初始化参数，这部分配置格式由各个Rpc后端类型定义，请参考对应Rpc后端类型的文档。
- `clients_options`和`servers_options`是一个规则列表，用于控制各个RPC方法在发起调用或处理调用时使用的Rpc后端规则，其中：
  - `func_name`表示本条规则的RPC方法名称，以正则表达式形式配置，如果RPC方法名称命中了该正则表达式，则会应用该条规则。
  - `enable_backends`是一个字符串数组，表示如果RPC方法名称命中了本条规则，则此数组就定义了该RPC方法能被处理的RPC后端。注意，该数组中出现的名称都必须要在`backends`中配置过。
  - 采用由上往下的顺序检查命中的规则，当某个RPC方法命中某条规则后，则不会针对此RPC方法再检查后面的规则。
- `client_filters`和`server_filters`是一个字符串数组，表示需要注册的client/server端框架侧过滤器列表，数组中的顺序为过滤器注册的顺序。一些插件会提供一些框架侧过滤器，用于在rpc调用时做一些前置/后置操作。


在AimRT中，RPC的前端接口和后端实现是解耦的，当开发者使用接口发起一个RPC调用，最终是要RPC后端来执行正真的RPC调用操作。

当Client端接口层发起一个RPC请求后，AimRT框架会根据以下规则，在多个RPC后端中选择一个进行实际的处理：
- AimRT框架会先根据`clients_options`配置确定某个RPC方法能被处理的RPC后端列表。
- AimRT框架会先解析传入的CTX里Meta参数中的`AIMRT_RPC_CONTEXT_KEY_TO_ADDR`项，如果其中手动配置了形如`xxx://yyy,zzz`这样的URL，则会解析出`xxx`字符串，并寻找同名的RPC后端进行处理。
- 如果没有配置CTX参数，则根据该RPC方法能被处理的RPC后端列表顺序，依次尝试进行处理，直到遇到第一个真正进行处理的后端。

Server端相对来说规则就比较简单，会根据`servers_options`的配置，接收并处理其中各个RPC后端传递过来的请求。



## local 类型Rpc后端


`local`类型的Rpc后端是AimRT官方提供的一种Rpc后端，用于请求同进程中的其他模块提供的RPC，它会自动判断Client端和Server端是否在同一个`Pkg`内，从而采用各种方式进行性能的优化。其所有的配置项如下：


| 节点                          | 类型      | 是否可选| 默认值 | 作用 |
| ----                          | ----      | ----  | ----  | ---- |
| timeout_executor              | string    | 可选  | ""    | Client端RPC超时情况下的执行器 |


以下是一个简单的示例：
```yaml
aimrt:
  executor:
    executors:
      - name: timeout_handle
        type: time_wheel
  rpc: # 【可选】RPC配置根节点
    backends: # 【可选】RPC后端列表
      - type: local # 【必选】RPC后端类型
        options: # 【可选】RPC Client配置
          timeout_executor: timeout_handle # 【可选】Client端RPC超时情况下的执行器
    clients_options: # 【可选】RPC Client配置
      - func_name: "(.*)" # 【必选】RPC Client名称，支持正则表达式
        enable_backends: [local] # 【必选】RPC Client允许使用的RPC后端列表
    servers_options: # 【可选】RPC Server配置
      - func_name: "(.*)" # 【必选】RPC Server名称，支持正则表达式
        enable_backends: [local] # 【必选】RPC Server允许使用的RPC后端列表
```

使用注意点如下：
- Server的执行器将使用Client调用时的执行器。同样，Client调用结束后的执行器将使用Server返回时的执行器。
- 如果client和server在一个Pkg中，那么Req、Rsp的传递将直接通过指针来进行；如果client和server在一个AimRT进程中，但在不同的Pkg里，那么Req、Rsp将会进行一次序列化/反序列化再进行传递。
- timeout功能仅在client和server位于不同Pkg时生效。如果client和server在一个Pkg中，那么为了保证Client端Req、Rsp的生命周期能覆盖Server端Req、Rsp的生命周期，timeout功能将不会生效。


