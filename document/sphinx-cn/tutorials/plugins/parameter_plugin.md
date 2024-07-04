# 参数插件设计



## 插件概述


**parameter_plugin**中注册了一个基于protobuf协议定义的RPC：`ParameterService`，提供了针对Parameter的一些管理接口，协议文件为[parameter.proto](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/protocols/plugins/parameter_plugin/parameter.proto)。请注意，**parameter_plugin**没有提供任何通信后端，因此本插件一般要搭配其他通信插件的RPC通信后端一块使用，例如[net_plugin](./net_plugin.md)中的http RPC后端。


在当前版本，本插件没有插件级的配置。以下是一个简单的配置示例，将**parameter_plugin**与**net_plugin**中的http RPC后端搭配使用：

```yaml
aimrt:
  plugin:
    plugins:
      - name: net_plugin # net插件，用于将parameter插件注册的RPC通过Http后端暴露给外部工具调用
        path: ./libaimrt_net_plugin.so
        options:
          thread_num: 4
          http_options:
            listen_ip: 127.0.0.1
            listen_port: 50080
      - name: parameter_plugin # parameter插件，没有任何额外配置
        path: ./libaimrt_parameter_plugin.so
  rpc:
    backends:
      - type: http
    servers_options:
      - func_name: "(.*)"
        enable_backends: [http]
```



## ParameterService

在[parameter.proto](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/protocols/plugins/parameter_plugin/parameter.proto)中，定义了一个`ParameterService`，提供了如下接口：
- Set：设置参数
- Get：获取参数
- List：列出参数列表
- Dump：导出所有参数
- Load：加载一份参数，可以直接加载之前Dump的参数


### Set

`Set`接口用于为某个模块设置/更新一个Key-Val参数对，其接口定义如下：
```proto
message SetParameterReq {
  string module_name = 1;
  string parameter_key = 2;
  string parameter_value = 3;
}

message SetParameterRsp {
  uint64 code = 1;
  string msg = 2;
}

service ParameterService {
  // ...
  rpc Set(SetParameterReq) returns (SetParameterRsp);
  // ...
}
```


以下是一个基于**net_plugin**中的http RPC后端，使用curl工具通过Http方式调用该接口的一个示例：
```shell
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_key": "key-1", "parameter_value": "val-abc"}'
```

该示例命令为`ParameterModule`这个模块添加/更新了一对K-V参数，参数Key为`key-1`，参数Val为`val-abc`。如果设置成功，该命令返回值如下：
```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 21

{"code":0,"msg":""}
```


### Get

`Get`接口用于从某个模块获取一个Key-Val参数对，其接口定义如下：
```proto
message GetParameterReq {
  string module_name = 1;
  string parameter_key = 2;
}

message GetParameterRsp {
  uint32 code = 1;
  string msg = 2;
  string parameter_value = 3;
}

service ParameterService {
  // ...
  rpc Get(GetParameterReq) returns (GetParameterRsp);
  // ...
}
```


以下是一个基于**net_plugin**中的http RPC后端，使用curl工具通过Http方式调用该接口的一个示例：
```shell
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Get' \
    -d '{"module_name": "ParameterModule", "parameter_key": "key-1"}'
```

该示例命令从`ParameterModule`这个模块中获取Key为`key-1`的参数的值。如果获取成功，该命令返回值如下：
```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 47

{"code":0,"msg":"","parameter_value":"val-abc"}
```


### List

`List`接口用于从某个模块获取所有参数的Key值，其接口定义如下：
```proto
message ListParameterReq {
  string module_name = 1;
}

message ListParameterRsp {
  uint32 code = 1;
  string msg = 2;
  repeated string parameter_keys = 3;
}

service ParameterService {
  // ...
  rpc List(ListParameterReq) returns (ListParameterRsp);
  // ...
}
```

以下是一个基于**net_plugin**中的http RPC后端，使用curl工具通过Http方式调用该接口的一个示例：
```shell
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/List' \
    -d '{"module_name": "ParameterModule"}'
```

该示例命令从`ParameterModule`这个模块中列出所有key的值。如果获取成功，该命令返回值如下：
```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 62

{"code":0,"msg":"","parameter_keys":["key-3","key-2","key-1"]}
```

### Dump


`Dump`接口用于从某个模块获取所有参数的Key和Val，其接口定义如下：
```proto
message ParameterMap {
  map<string, string> value = 1;
}

message DumpParameterReq {
  repeated string module_names = 1;
}

message DumpParameterRsp {
  uint32 code = 1;
  string msg = 2;

  map<string, ParameterMap> module_parameter_map = 3;  // key: module_name
}

service ParameterService {
  // ...
  rpc Dump(DumpParameterReq) returns (DumpParameterRsp);
  // ...
}
```


该接口支持一次性从多个模块中Dump所有数据。以下是一个基于**net_plugin**中的http RPC后端，使用curl工具通过Http方式调用该接口的一个示例：
```shell
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Dump' \
    -d '{"module_names": ["ParameterModule"]}'
```

该示例命令从`ParameterModule`这个模块中导出所有的参数。如果导出成功，该命令返回值如下：
```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 218

{
	"code": 0,
	"msg": "",
	"module_parameter_map": {
		"ParameterModule": {
			"value": {
				"key-3": "val-3",
				"key-9": "val-9",
				"key-8": "val-8",
				"key-4": "val-4",
				"key-5": "val-5",
				"key-2": "val-2",
				"key-1": "val-1",
				"key-7": "val-7",
				"key-6": "val-6"
			}
		}
	}
}
```


### Load


`Load`接口用于向某些模块导入一份参数包，这个参数包可以是`Dump`接口导出的，其接口定义如下：
```proto
message ParameterMap {
  map<string, string> value = 1;
}

message LoadParameterReq {
  map<string, ParameterMap> module_parameter_map = 1;  // key: module_name
}

message LoadParameterRsp {
  uint32 code = 1;
  string msg = 2;
}

service ParameterService {
  // ...
  rpc Load(LoadParameterReq) returns (LoadParameterRsp);
  // ...
}
```


该接口支持一次性向多个模块中导入数据。以下是一个基于**net_plugin**中的http RPC后端，使用curl工具通过Http方式调用该接口的一个示例：
```shell
#!/bin/bash

data='{
	"module_parameter_map": {
		"ParameterModule": {
			"value": {
				"key-a": "val-111",
				"key-b": "val-222",
				"key-c": "val-333",
				"key-d": "val-444",
				"key-e": "val-555",
				"key-f": "val-666",
				"key-g": "val-777"
			}
		}
	}
}'


curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Load' \
    -d "$data"
```

该示例命令向`ParameterModule`这个模块中导入一份参数。如果导入成功，该命令返回值如下：
```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 19

{"code":0,"msg":""}
```

