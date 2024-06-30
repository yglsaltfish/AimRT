
# 日志动态控制插件





**log_control_plugin**中注册了一个基于protobuf协议定义的RPC：`LogControlService`，提供了针对Log的一些运行时管理接口，协议文件为[log_control.proto](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/protocols/plugins/log_control_plugin/log_control.proto)。请注意，**log_control_plugin**没有提供任何通信后端，因此本插件一般要搭配其他通信插件的RPC通信后端一块使用，例如[net_plugin](./net_plugin.md)中的http RPC后端。



在当前版本，本插件没有插件级的配置。以下是一个简单的配置示例，将**log_control_plugin**与**net_plugin**中的http RPC后端搭配使用：


```yaml
aimrt:
  plugin:
    plugins:
      - name: net_plugin # net插件，用于将parameter插件注册的RPC通过Http后端暴露给外部工具调用
        path: ./libnet_plugin.so
        options:
          thread_num: 4
          http_options:
            listen_ip: 127.0.0.1
            listen_port: 50080
      - name: log_control_plugin # parameter插件，没有任何额外配置
        path: ./liblog_control_plugin.so
  rpc:
    backends:
      - type: http
    servers_options:
      - func_name: "(.*)"
        enable_backends: [http]
```


## LogControlService

在[log_control.proto](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/protocols/plugins/log_control_plugin/log_control.proto)中，定义了一个`LogControlService`，提供了如下接口：
- GetModuleLogLevel：获取模块日志等级
- SetModuleLogLevel：设置模块日志等级


### GetModuleLogLevel

`GetModuleLogLevel`接口用于获取某个模块的日志等级，其接口定义如下：
```proto
message GetModuleLogLevelReq {
  repeated string module_names = 1;  // if empty, then get all module
}

message GetModuleLogLevelRsp {
  uint32 code = 1;
  string msg = 2;

  map<string, string> module_log_level_map = 3;  // key: module_name
}

service ParameterService {
  // ...
  rpc GetModuleLogLevel(GetModuleLogLevelReq) returns (GetModuleLogLevelRsp);
  // ...
}
```

开发者在请求包`GetModuleLogLevelReq`中填入想要查询日志等级的模块。如果为空则返回所有模块。


以下是一个基于**net_plugin**中的http RPC后端，使用curl工具通过Http方式调用该接口的一个示例：
```shell
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.log_control_plugin.LogControlService/GetModuleLogLevel' \
    -d '{"module_names": []}'
```

该示例命令查询当前所有模块的日志等级，如果调用成功，该命令返回值如下：
```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 84

{"code":0,"msg":"","module_log_level_map":{"core":"Info","ExecutorCoModule":"Info"}}
```


### SetModuleLogLevel



`SetModuleLogLevel`接口用于设置某个或某些模块的日志等级，其接口定义如下：
```proto
message SetModuleLogLevelReq {
  map<string, string> module_log_level_map = 1;
}

message SetModuleLogLevelRsp {
  uint32 code = 1;
  string msg = 2;
}


service ParameterService {
  // ...
  rpc SetModuleLogLevel(SetModuleLogLevelReq) returns (SetModuleLogLevelRsp);
  // ...
}
```

开发者在请求包`SetModuleLogLevelReq`中填入想要设置日志等级的模块以及对应的日志等级。


以下是一个基于**net_plugin**中的http RPC后端，使用curl工具通过Http方式调用该接口的一个示例：
```shell
#!/bin/bash

data='{
	"module_log_level_map": {
		"core": "Trace",
		"ExecutorCoModule": "Trace"
	}
}'

curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.log_control_plugin.LogControlService/SetModuleLogLevel' \
    -d "$data"
```

该示例命令为`core`和`ExecutorCoModule`模块设置了`Trace`的日志等级，如果调用成功，该命令返回值如下：
```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 19

{"code":0,"msg":""}
```
