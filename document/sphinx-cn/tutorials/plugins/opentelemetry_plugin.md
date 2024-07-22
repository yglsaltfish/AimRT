
# opentelemetry插件

## 插件概述


**opentelemetry_plugin**是一个基于[OpenTelemetry](https://opentelemetry.io/)的插件，为AimRT提供框架层面的可观测性功能。它提供了一些RPC Framework Filter：
- client filter:
  - otp_trace：用于进行client端rpc链路追踪
- server filter:
  - otp_trace：用于进行server端rpc链路追踪


插件的配置项如下：

| 节点                      | 类型      | 是否可选| 默认值          | 作用 |
| ----                      | ----      | ----  | ----              | ---- |
| node_name                 | string    | 必选  | ""                | 上报时的节点名称，不可为空 |
| otlp_http_exporter_url    | string    | 必选  | ""                | 基于otlp http exporter上报时的url |
| attributes                | array     | 可选  | []                | kv属性数组 |
| attributes[i].key         | string    | 必选  | ""                | 属性的key值 |
| attributes[i].val         | string    | 必选  | ""                | 属性的val值 |


在配置了插件后，还需要在`rpc`节点下的的`client_filters`和`server_filters`配置中注册`otp_trace`类型的过滤器，才能在rpc调用前后进行trace跟踪。以下是一个简单的示例：
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
```

## 常用实践

OpenTelemetry的自身定位很明确：数据采集和标准规范的统一，对于数据如何去使用、存储、展示、告警，官方是不涉及的，我们目前推荐使用Prometheus + Grafana做Metrics存储、展示，使用Jaeger做分布式跟踪的存储和展示。

此外，如果每一个服务都单独去上报，会造成性能上的浪费，在生产实践中一般是用一个本地的collector，收集本地所有的上报信息，然后再统一上报到远端平台。

### Jaeger

[Jaeger](https://www.jaegertracing.io/)是一个兼容opentelemetry上报标准的分布式跟踪、分析平台，可以简单的使用以下命令启动一个Jaeger docker实例：
```shell
docker run -d \
  -e COLLECTOR_ZIPKIN_HOST_PORT=:9411 \
  -p 16686:16686 \
  -p 4317:4317 \
  -p 4318:4318 \
  -p 9411:9411 \
  jaegertracing/all-in-one:latest
```

启动之后，即可将opentelemetry插件的otlp_http_exporter_url配置指向Jaeger所开的4318端口，从而将trace信息上报到Jaeger平台上。

