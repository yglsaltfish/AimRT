# Rpc

## RpcHandleRef：RPC句柄

AimRT Python接口当前只支持protobuf形式的协议。AimRT提供了一个protoc插件，来生成基于protobuf的rpc桩代码。


AimRT中，模块可以通过调用`CoreRef`句柄的`GetRpcHandle()`接口，获取`RpcHandleRef`句柄。开发者在使用RPC功能时必须要按照一定的步骤，调用`RpcHandleRef`中的几个核心接口：
- Client端：
  - 在`Initialize`阶段，调用**注册RPC Client方法**的接口；
  - 在`Start`阶段，调用**RPC Invoke**的接口，以实现RPC调用；
- Server端：
  - 在`Initialize`阶段，**注册RPC Server服务**的接口；

一般情况下，使用者不会直接使用`RpcHandleRef`直接提供的那些接口，而是根据RPC IDL文件生成一些桩代码，对`RpcHandleRef`句柄进行一些封装，然后在业务代码中使用这些桩代码提供的接口。


此外，在RPC调用或者RPC处理时，使用者还可以通过一个`status`变量获取RPC请求时框架的错误情况，其中最主要的是一个错误码字段。

## 基于protobuf生成python桩代码


[Protobuf](https://protobuf.dev/)（Protocol Buffers）是一种由Google开发的用于序列化结构化数据的轻量级、高效的数据交换格式，是一种广泛使用的IDL。它不仅能够描述消息结构，还提供了`service`语句来定义RPC服务。


在使用时，开发者需要先定义一个`.proto`文件，在其中定义一个RPC服务。例如`rpc.proto`：

```protobuf
syntax = "proto3";

message ExampleReq {
  string msg = 1;
  int32 num = 2;
}

message ExampleRsp {
  uint64 code = 1;
  string msg = 2;
}

service ExampleService {
  rpc ExampleFunc(ExampleReq) returns (ExampleRsp);
}
```

然后使用Protobuf官方提供的protoc工具进行转换，生成消息结构部分的Python接口，例如：
```shell
protoc --python_out=. rpc.proto
```

这将生成`rpc_pb2.py`文件，包含了根据定义的消息类型生成的Python接口。


在这之后，还需要使用AimRT提供的protoc插件，生成服务定义部分的Python桩代码，例如：
```shell
protoc --aimrt_rpc_out=. --plugin=protoc-gen-aimrt_rpc=./protoc_plugin_py_gen_aimrt_py_rpc.py rpc.proto
```

这将生成`rpc_aimrt_rpc_pb2.py`文件，包含了根据定义的服务生成的Python接口。

## Client接口

AimRT Python接口中，提供的RPC Client接口都是同步型接口，在发起请求后会阻塞当前线程，直到收到回包或请求超时。使用该接口发起RPC调用非常简单，一般分为以下几个步骤：
- Step0：引用根据protobuf协议生成的桩代码包，例如`xxx_aimrt_rpc_pb2.py`；
- Step1：在`Initialize`阶段调用`RegisterClientFunc`方法注册RPC Client；
- Step2：在`Start`阶段里某个业务函数里发起RPC调用：
  - Step2-1：创建一个`Proxy`，构造参数是`RpcHandleRef`。proxy非常轻量，可以随用随创建；
  - Step2-2：创建Req、Rsp，并填充Req内容；
  - Step2-3：【可选】创建ctx，设置超时等信息；
  - Step2-4：基于proxy，传入ctx、Req、Rsp，发起RPC调用，同步等待RPC调用结束，获取返回的status；
  - Step2-5：解析status和Rsp；


以下是一个简单的示例：

```python
import aimrt_py
import aimrt_py_log

import rpc_pb2
import rpc_aimrt_rpc_pb2

from google.protobuf.json_format import MessageToJson


class NormalRpcClientModule(aimrt_py.ModuleBase):
    def __init__(self):
        super().__init__()
        self.logger = aimrt_py.LoggerRef()
        self.rpc_handle = aimrt_py.RpcHandleRef()
        self.proxy = rpc_aimrt_rpc_pb2.ExampleServiceProxy()

    def Info(self):
        info = aimrt_py.ModuleInfo()
        info.name = "NormalRpcClientModule"
        return info

    def Initialize(self, core):
        self.logger = core.GetLogger()
        aimrt_py_log.info(self.logger, "Module initialize")

        # rpc-client
        self.rpc_handle = core.GetRpcHandle()
        ret = rpc_aimrt_rpc_pb2.ExampleServiceProxy.RegisterClientFunc(self.rpc_handle)
        if (not ret):
            aimrt_py_log.error(self.logger, "Register client failed.")
            return False

        self.proxy = rpc_aimrt_rpc_pb2.ExampleServiceProxy(self.rpc_handle)
 
        return True

    def Start(self):
        aimrt_py_log.info(self.logger, "Module start")

        # call rpc
        req = rpc_pb2.GetFooDataReq()
        req.msg = "hello world"

        ctx = self.rpc_handle.NewContextSharedPtr()
        ctx_ref = aimrt_py.RpcContextRef(ctx)
        status, rsp = self.proxy.GetFooData(ctx_ref, req)

        # check rsp
        if (status):
            aimrt_py_log.info(
                self.logger, "Client get rpc ret, status: {}, rsp: {}".format(
                    status.ToString(), MessageToJson(rsp)))
        else:
            aimrt_py_log.warn(self.logger,
                                "Client get rpc error ret, status: {}".format(status.ToString()))

        return True

    def Shutdown(self):
        aimrt_py_log.info(self.logger, "Module shutdown")
```


## Server接口


AimRT Python接口中，提供的RPC Server接口都是同步型接口，必须在当前handle中返回最终的返回包。使用该接口实现RPC服务，一般分为以下几个步骤：
- Step0：引用桩代码头文件，例如`xxx_aimrt_rpc_pb2.py`；
- Step1：开发者实现一个Impl类，继承包中的`XXXService`，并实现其中的虚接口；
  - Step1-1：解析Req，并填充Rsp；
  - Step1-2：返回`Status`；
- Step2：在`Initialize`阶段调用`RegisterService`方法注册RPC Service；


以下是一个简单的示例：

```python
import aimrt_py
import aimrt_py_log

import rpc_pb2
import rpc_aimrt_rpc_pb2

from google.protobuf.json_format import MessageToJson


class ExampleServiceImpl(rpc_aimrt_rpc_pb2.ExampleService):
    def __init__(self, logger):
        super().__init__()
        self.logger = logger

    def GetFooData(self, ctx, req):
        rsp = rpc_pb2.GetFooDataRsp()
        rsp.msg = "echo " + req.msg

        aimrt_py_log.info(self.logger,
                          "Server handle new rpc call. req: {}, return rsp: {}"
                          .format(MessageToJson(req), MessageToJson(rsp)))

        return aimrt_py.RpcStatus(), rsp

    def GetBarData(self, ctx, req):
        rsp = rpc_pb2.GetBarDataRsp()
        rsp.msg = "echo " + req.msg

        aimrt_py_log.info(self.logger,
                          "Server handle new rpc call. req: {}, return rsp: {}"
                          .format(MessageToJson(req), MessageToJson(rsp)))

        return aimrt_py.RpcStatus(), rsp


class NormalRpcServerModule(aimrt_py.ModuleBase):
    def __init__(self):
        super().__init__()
        self.logger = aimrt_py.LoggerRef()

    def Info(self):
        info = aimrt_py.ModuleInfo()
        info.name = "NormalRpcServerModule"
        return info

    def Initialize(self, core):
        self.logger = core.GetLogger()

        # log
        aimrt_py_log.info(self.logger, "Module initialize")

        # rpc-server
        self.service = ExampleServiceImpl(self.logger)
        ret = core.GetRpcHandle().RegisterService(self.service)
        if (not ret):
            aimrt_py_log.error(self.logger, "Register service failed.")
            return False

        return True

    def Start(self):
        aimrt_py_log.info(self.logger, "Module start")

        return True

    def Shutdown(self):
        aimrt_py_log.info(self.logger, "Module shutdown")
```
