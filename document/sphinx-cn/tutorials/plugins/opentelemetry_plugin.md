
# opentelemetry插件

## 插件概述


**opentelemetry_plugin**是一个基于[OpenTelemetry](https://opentelemetry.io/)的插件，为 AimRT 提供框架层面的可观测性功能。它提供了一些 RPC/Channel Framework Filter：
- client filter:
  - otp_trace：用于进行 client 端 rpc 链路追踪
- server filter:
  - otp_trace：用于进行 server 端 rpc 链路追踪
- Publish filter:
  - otp_trace：用于进行 publish 端 channel 链路追踪
- Subscribe filter:
  - otp_trace：用于进行 subscribe 端 channel 链路追踪


插件的配置项如下：

| 节点                      | 类型      | 是否可选| 默认值      | 作用 |
| ----                      | ----      | ----  | ----        | ---- |
| node_name                 | string    | 必选  | ""          | 上报时的节点名称，不可为空 |
| otlp_http_exporter_url    | string    | 必选  | ""          | 基于 otlp http exporter 上报时的 url |
| attributes                | array     | 可选  | []          | kv 属性数组 |
| attributes[i].key         | string    | 必选  | ""          | 属性的 key 值 |
| attributes[i].val         | string    | 必选  | ""          | 属性的 val 值 |


在配置了插件后，还需要在`rpc`/`channel`节点下的的 filter 配置中注册`otp_trace`类型的过滤器，才能在rpc/channel调用前后进行trace跟踪。以下是一个简单的示例：
```yaml
aimrt:
  plugin:
    plugins:
      - name: opentelemetry_plugin
        path: ./libaimrt_opentelemetry_plugin.so
        options:
          node_name: example_node # 【必选】上报时的节点名称，不可为空
          otlp_http_exporter_url: http://localhost:4318/v1/traces # 【必选】基于otlp http exporter上报时的url
          attributes: # 【可选】kv属性数组
            - key: sn # 【必选】属性的key值
              val: 123456 # 【必选】属性的val值
  rpc:
    backends:
      - type: local # 此处以local后端为例
    clients_options:
      - func_name: "(.*)"
        enable_backends: [local]
    servers_options:
      - func_name: "(.*)"
        enable_backends: [local]
    client_filters: [otp_trace] # 注册client端的otp_trace过滤器
    server_filters: [otp_trace] # 注册server端的otp_trace过滤器
  channel:
    backends:
      - type: local # 此处以local后端为例
        options:
          subscriber_use_inline_executor: false
          subscriber_executor: work_thread_pool
    pub_topics_options:
      - topic_name: "(.*)"
        enable_backends: [local]
    sub_topics_options:
      - topic_name: "(.*)"
        enable_backends: [local]
    pub_filters: [otp_trace] # 注册pub端的otp_trace过滤器
    sub_filters: [otp_trace] # 注册sub端的otp_trace过滤器
  module:
    # ...
```


在完成配置后，使用者还需要在希望进行链路追踪的地方设置 RPC/Channel 的 Context，分为两种情况：

1. 从一个 RPC Clinet 或一个 Channel Publish 强制开启链路追踪，此时需要向 Context 的 Meta 信息中设置`aimrt_otp-start_new_trace`为`True`，例如：
  - RPC:
  ```cpp
  auto ctx_ptr = client_proxy->NewContextSharedPtr();
  ctx_ptr->SetMetaValue("aimrt_otp-start_new_trace", "True");

  auto status = co_await client_proxy->GetFooData(ctx_ptr, req, rsp);
  // ...
  ```
  - Channel:
  ```cpp
  auto ctx_ptr = publisher_proxy.NewContextSharedPtr();
  ctx_ptr->SetMetaValue("aimrt_otp-start_new_trace", "True");

  publisher_proxy.Publish(ctx_ptr, msg);
  // ...
  ```

2. 从一个 RPC Clinet 或一个 Channel Publish 跟随上层 RPC Server/Channel Subscribe 继续追踪一个链路，此时需要继承上游的 RPC Server/Channel Subscribe 的 Context，例如：
  - RPC:
  ```cpp
  // RPC Server Handle
  co::Task<rpc::Status> GetFooData(rpc::ContextRef server_ctx, const GetFooDataReq& req, GetFooDataRsp& rsp) {
    // ...

    // 继承上游 Server 的 Context 信息
    auto client_ctx_ptr = client_proxy->NewContextSharedPtr(server_ctx);

    auto status = co_await client_proxy->GetFooData(client_ctx_ptr, req, rsp);
    // ...
  }

  ```
  - Channel:
  ```cpp
  // Channel Subscribe Handle
  void EventHandle(channel::ContextRef subscribe_ctx, const std::shared_ptr<const ExampleEventMsg>& data) {
    // ...

    // 继承上游 Subscribe 的 Context 信息
    auto publishe_ctx = publisher_proxy.NewContextSharedPtr(subscribe_ctx);

    publisher_proxy.Publish(publishe_ctx, msg);
    // ...
  }
  ```


## 常用实践

OpenTelemetry 的自身定位很明确：数据采集和标准规范的统一，对于数据如何去使用、存储、展示、告警，官方是不涉及的，我们目前推荐使用 Prometheus + Grafana 做 Metrics 存储、展示，使用 Jaeger 做分布式跟踪的存储和展示。

此外，如果每一个服务都单独去上报，会造成性能上的浪费，在生产实践中一般是用一个本地的 collector ，收集本地所有的上报信息，然后再统一上报到远端平台。

### collector

***TODO待完善***

### Jaeger

[Jaeger](https://www.jaegertracing.io/)是一个兼容 opentelemetry 上报标准的分布式跟踪、分析平台，可以简单的使用以下命令启动一个 Jaeger docker 实例：
```shell
docker run -d \
  -e COLLECTOR_ZIPKIN_HOST_PORT=:9411 \
  -p 16686:16686 \
  -p 4317:4317 \
  -p 4318:4318 \
  -p 9411:9411 \
  jaegertracing/all-in-one:latest
```

启动之后，即可将 opentelemetry 插件的 otlp_http_exporter_url 配置指向 Jaeger 所开的 4318 端口，从而将 trace 信息上报到 Jaeger 平台上。可以访问 Jaeger 在 16686 端口上的 web 页面查看 trace 信息。

