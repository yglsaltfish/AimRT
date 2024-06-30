
# Module接口-Python版本


## 简介

AimRT为逻辑实现阶段的`Module`开发提供了一套Python接口层，这层接口是基于CPP接口层、使用pybind11封装的，其接口风格与CPP接口非常类似。

注意，如未作特殊说明，本文档中所有的类型都是`aimrt_py`包中的。

## 一些通用性说明


### 接口中的引用类型

- 大部分句柄是一种引用类型，类型命名一般以`Ref`结尾。
- 这种引用类型一般比较轻量，拷贝传递不会有较大开销。
- 这种引用类型一般都提供了一个`operator bool()`的重载来判断引用是否有效。
- 调用这种引用类型中提供的接口时，如果引用为空，会抛出一个异常。


### AimRT生命周期以及接口调用时机


参考[接口概述](interface.md)，AimRT在运行时有三个主要的阶段：Initialize阶段、Start阶段、Shutdown阶段，一些接口只能在其中一些阶段里被调用。本文档中，如果不对接口做特殊说明，则默认该接口在所有阶段都可以被调用。


### 大部分接口的实际表现需要根据部署运行配置而定

在逻辑实现阶段，开发者只需要知道此接口在抽象意义上代表什么功能即可，至于在实际运行时的表现，则要根据部署运行配置而定，在逻辑开发阶段也不应该关心太多。例如，开发者可以使用log接口打印一行日志，但这个日志最终打印到文件里还是控制台上，则需要根据运行时配置而定，开发者在写业务逻辑时不需要关心。


### 支持的协议

AimRT Python接口目前只支持Protobuf协议，这与CPP接口有所不同。后续有可能会支持ROS2协议，但这将牵涉到ROS2的引用，可能会引入较重的依赖负担。


## ModuleBase：模块基类


业务模块需要继承`ModuleBase`基类，它定义了业务模块所需要实现的几个接口，包括：
- `Info()->ModuleInfo`：用于AimRT框架获取模块信息，包括模块名称、模块版本等。
  - AimRT框架会在加载模块时调用此接口，读取模块信息。
  - `ModuleInfo`结构中除`name`是必须项，其余都是可选项。
  - 如果模块在其中抛了异常，等效于返回一个空ModuleInfo。
- `Initialize(CoreRef)->bool`：用于初始化模块。
  - AimRT框架在Initialize阶段，依次调用各模块的Initialize方法。
  - AimRT框架保证在主线程中调用模块的Initialize方法，模块不应阻塞Initialize方法太久。
  - AimRT框架在调用模块Initialize方法时，会传入一个CoreRef句柄，模块可以存储此句柄，并在后续通过它调用框架的功能。
  - 在AimRT框架调用模块的Initialize方法之前，保证所有的组件（例如配置、日志等）都已经完成Initialize，但还未Start。
  - 如果模块在Initialize方法中抛了异常，等效于返回false。
  - 如果有任何模块在AimRT框架调用其Initialize方法时返回了false，则整个AimRT框架会Initialize失败。
- `Start()->bool`：用于启动模块。
  - AimRT框架在Start阶段依次调用各模块的Start方法。
  - AimRT框架保证在主线程中调用模块的Start方法，模块不应阻塞Start方法太久。
  - 在AimRT框架调用模块的Start方法之前，保证所有的组件（例如配置、日志等）都已经进入Start阶段。
  - 如果模块在Start方法中抛了异常，等效于返回了false。
  - 如果有任何模块在AimRT框架调用其Start方法时返回了false，则整个AimRT框架会Start失败。
- `Shutdown()`：用于停止模块，一般用于整个进程的优雅退出。
  - AimRT框架在Shutdown阶段依次调用各个模块的Shutdown方法。
  - AimRT框架保证在主线程中调用模块的Shutdown方法，模块不应阻塞Shutdown方法太久。
  - AimRT框架可能在任何阶段直接进入Shutdown阶段。
  - 如果模块在Shutdown方法中抛了异常，框架会catch住并直接返回。
  - 在AimRT框架调用模块的Shutdown方法之后，各个组件（例如配置、日志等）才会Shutdown。



`ModuleInfo`类型有以下成员，其中除了`name`是必选的，其他都是可选项：
- name(str)
- major_version(int)
- minor_version(int)
- patch_version(int)
- build_version(int)
- author(str)
- description(str)


  
以下是一个简单的示例，实现了一个最基础的HelloWorld模块：
```python
import aimrt_py

class HelloWorldModule(aimrt_py.ModuleBase):
    def __init__(self):
        super().__init__()

    # 返回本模块的一些信息
    def Info(self):
        info = aimrt_py.ModuleInfo()
        info.name = "HelloWorldModule"
        return info

    # 框架初始化时会调用此方法
    def Initialize(self, core):
        print("Initialize")
        return True

    # 框架启动时会调用此方法
    def Start(self):
        print("Start")
        return True

    # 框架停止时会调用此方法
    def Shutdown(self):
        print("Shutdown")
```


## CoreRef：框架句柄

在模块的`Initialize`方法中，AimRT框架会传入一个`CoreRef`句柄，模块通过该句柄的一些接口调用框架的功能。`CoreRef`中提供的核心接口如下：
- `GetConfigurator()->ConfiguratorRef` : 获取配置句柄
- `GetLogger()->LoggerRef` ： 获取日志句柄
- `GetExecutorManager()->ExecutorManagerRef` ： 获取执行器句柄
- `GetRpcHandle()->RpcHandleRef` ： 获取RPC句柄
- `GetChannelHandle()->ChannelHandleRef` ： 获取Channel句柄


关于`CoreRef`的使用注意点如下：
- AimRT框架会为每个模块生成一个专属`CoreRef`句柄，以实现资源隔离、监控等方面的功能。
- 模块通过`CoreRef`中的接口获取对应组件的句柄，并通过它们来调用相关功能。

一个简单的示例如下：
```python
import aimrt_py
import aimrt_py_log

class HelloWorldModule(aimrt_py.ModuleBase):
    def Initialize(self, core):
        # 获取日志句柄
        logger = core.GetLogger()

        # 使用日志句柄打印日志
        aimrt_py_log.info(logger, "This is a test log")
        return True
```

## ConfiguratorRef：配置句柄


模块可以通过调用`CoreRef`句柄的`GetConfigurator()`接口，获取`ConfiguratorRef`句柄，通过其使用一些配置相关的功能。其提供的核心接口如下：
- `GetConfigFilePath()->str` : 获取配置文件路径


使用注意点如下：
- `GetConfigFilePath()->str`接口：用于获取模块配置文件的路径。
  - 请注意，此接口仅返回一个模块配置文件的路径，模块开发者需要自己读取配置文件并解析。
  - 这个接口具体会返回什么样的路径，请参考部署运行阶段[配置](cfg.md)文档中的`aimrt.module`章节。


一个简单的使用示例如下：
```python
import aimrt_py
import yaml

class HelloWorldModule(aimrt_py.ModuleBase):
    def Initialize(self, core):
        # 获取配置句柄
        configurator = core.GetConfigurator()

        # 获取配置路径
        cfg_file_path = configurator.GetConfigFilePath()

        # 根据用户实际使用的文件格式来解析配置文件。本例中基于yaml来解析
        with open(cfg_file_path, 'r') as file:
            data = yaml.safe_load(file)
            # ...

        return True
```

## ExecutorManagerRef：执行器句柄

执行器的具体概念请参考[CPP模块接口](./cpp_module.md)中的介绍。此章节仅介绍具体的语法。模块可以通过调用`CoreRef`句柄的`GetExecutorManager()`接口，获取`ExecutorManagerRef`句柄，其中提供了一个简单的获取Executor的接口：
- `GetExecutor()->ExecutorRef` : 获取执行器句柄


使用者可以调用`ExecutorManagerRef`中的`GetExecutor`方法，获取指定名称的`ExecutorRef`句柄，以调用执行器相关功能。`ExecutorRef`的核心接口如下：

- `Type()->str`：获取执行器的类型。
- `Name()->str`：获取执行器的名称。
- `ThreadSafe()->bool`：返回本执行器是否是线程安全的。
- `IsInCurrentExecutor()->bool`：判断调用此函数时是否在本执行器中。
  - 注意：如果返回true，则当前环境一定在本执行器中；如果返回false，则当前环境有可能不在本执行器中，也有可能在。
- `SupportTimerSchedule()->bool`：返回本执行器是否支持按时间调度的接口，也就是`ExecuteAt`、`ExecuteAfter`接口。
- `Execute(func)`：将一个任务投递到本执行器中，并在调度后立即执行。
  - 参数`Task`简单的视为一个满足`std::function<void()>`签名的任务闭包。
  - 此接口可以在Initialize/Start/Shutdown阶段调用，但执行器在Start阶段后才开始执行，因此在Start阶段之前调用此接口只能将任务投递到执行器的任务队列中而不会执行，等到Start之后才能开始执行任务。
- `Now()->datetime`：获取本执行器体系下的时间。
  - 对于一般的执行器来说，此处返回的都是`std::chrono::system_clock::now()`的结果。
  - 有一些带时间调速功能的特殊执行器，此处可能会返回经过处理的时间。
- `ExecuteAt(datetime, func)`：在某个时间点执行一个任务。
  - 第一个参数-时间点，以本执行器的时间体系为准。
  - 参数`Task`简单的视为一个满足`std::function<void()>`签名的任务闭包。
  - 如果本执行器不支持按时间调度，则调用此接口时会抛出一个异常。
  - 此接口可以在Initialize/Start/Shutdown阶段调用，但执行器在Start阶段后才开始执行，因此在Start阶段之前调用此接口只能将任务投递到执行器的任务队列中而不会执行，等到Start之后才能开始执行任务。
- `ExecuteAfter(timedelta, func)`：在某个时间后执行一个任务。
  - 第一个参数-时间段，以本执行器的时间体系为准。
  - 参数`Task`简单的视为一个满足`std::function<void()>`签名的任务闭包。
  - 如果本执行器不支持按时间调度，则调用此接口时会抛出一个异常。
  - 此接口可以在Initialize/Start/Shutdown阶段调用，但执行器在Start阶段后才开始执行，因此在Start阶段之前调用此接口只能将任务投递到执行器的任务队列中而不会执行，等到Start之后才能开始执行任务。


以下是一个简单的使用示例，演示了如何获取一个执行器句柄，并将一个简单的任务投递到该执行器中执行：
```python
import aimrt_py
import aimrt_py_log
import datetime

class HelloWorldModule(aimrt_py.ModuleBase):
    def __init__(self):
        super().__init__()
        self.logger = aimrt_py.LoggerRef()
        self.work_executor = aimrt_py.ExecutorRef()

    def Initialize(self, core):
        self.logger = core.GetLogger()

        # 获取执行器
        self.work_executor = core.GetExecutorManager().GetExecutor("work_thread_pool")
        return True

    def Start(self):
        # 示例任务
        def test_task():
            aimrt_py_log.info(self.logger, "run test task.")

        # 投递到执行器中并立即执行
        self.work_executor.Execute(test_task)

        # 投递到执行器中并在一段时间后执行
        self.work_executor.ExecuteAfter(datetime.timedelta(seconds=1), test_task)

        # 投递到执行器中并在某个时间点执行
        self.work_executor.ExecuteAt(datetime.datetime.now() + datetime.timedelta(seconds=2), test_task)

        return True
```


## LoggerRef：日志句柄

AimRT提供了`aimrt_py_log`包，来封装log接口，其作用相当于CPP接口中的日志宏。在AimRT中，模块可以通过调用`CoreRef`句柄的`GetLogger()`接口，获取`LoggerRef`句柄，这个句柄可以直接作为`aimrt_py_log`中日志接口的参数，来实现日志功能。


模块开发者可以直接参照以下示例的方式，使用分配给模块的日志句柄来打印日志：
```python
import aimrt_py
import aimrt_py_log

class HelloWorldModule(aimrt_py.ModuleBase):
    def Initialize(self, core):
        # 获取日志
        logger = core.GetLogger()

        # 打印日志
        aimrt_py_log.trace(logger, "This is a test trace log")
        aimrt_py_log.debug(logger, "This is a test trace log")
        aimrt_py_log.info(logger, "This is a test trace log")
        aimrt_py_log.warn(logger, "This is a test trace log")
        aimrt_py_log.error(logger, "This is a test trace log")
        aimrt_py_log.fatal(logger, "This is a test trace log")

        return True
```

## ChannelHandleRef：Channel句柄

AimRT Python接口当前只支持protobuf形式的协议。AimRT提供了`aimrt_py_pb_chn`包，来提供基于protobuf的channel功能。

AimRT中，模块可以通过调用`CoreRef`句柄的`GetChannelHandle()`接口，获取`ChannelHandleRef`句柄，来使用Channel功能。其提供的核心接口如下：
- `GetPublisher(str)->PublisherRef`
- `GetSubscriber(str)->SubscriberRef`


使用者可以调用`ChannelHandleRef`中的`GetPublisher`方法和`GetSubscriber`方法，获取**指定Topic名称**的`PublisherRef`句柄和`SubscriberRef`句柄，分别用于Channel发布和订阅。关于这两个方法，有一些使用时的须知：
  - 这两个接口是线程安全的。
  - 这两个接口可以在`Initialize`阶段和`Start`阶段使用。


### 基于protobuf生成python桩代码

在继续使用AimRT Python发送/订阅消息之前，使用者需要先基于protobuf协议生成一些python的桩代码。

[Protobuf](https://protobuf.dev/)（Protocol Buffers）是一种由Google开发的用于序列化结构化数据的轻量级、高效的数据交换格式，是一种广泛使用的IDL。它类似于XML和JSON，但更为紧凑、快速、简单，且可扩展性强。

在使用时，开发者需要先定义一个`.proto`文件，在其中定义一个消息结构。例如`example.proto`：

```protobuf
syntax = "proto3";

message ExampleMsg {
  string msg = 1;
  int32 num = 2;
}
```

然后使用Protobuf官方提供的protoc工具进行转换，生成Python桩代码，例如：
```shell
protoc --python_out=. example.proto
```

这将生成`example_pb2.py`文件，包含了根据定义的消息类型生成的Python接口。


### Pub接口

用户如果需要发布一个Msg，牵涉的接口主要有以下两个：
- `aimrt_py_pb_chn.RegisterPublishType(publisher, msg_type)->bool` ： 用于注册此消息类型
- `aimrt_py_pb_chn.Publish(publisher, msg)` ： 用于发布消息



用户需要两个步骤来实现逻辑层面的消息发布：
- Step1：使用`aimrt_py_pb_chn.RegisterPublishType`方法注册协议类型；
  - 只能在`Initialize`阶段注册；
  - 不允许在一个`publisher`中重复注册同一种类型；
  - 如果注册失败，会返回false；
- Step2：使用`Publish`方法发布数据；
  - 只能在`Start`阶段之后发布数据；


用户在逻辑层`Publish`一个消息后，特定的Channel后端将处理具体的消息发布请求，此时根据后端的表现，有可能会阻塞一段时间，因此`Publish`方法耗费的时间是未定义的。但一般来说，Channel后端都不会阻塞`Publish`方法太久，详细的信息请参考您使用的后端的文档。

以下是一个示例，基于`Module`模式在`Initialize`方法中拿到`CoreRef`句柄，如果在App模式下Create Module方式拿到`CoreRef`句柄，使用方法也类似：
```python
import aimrt_py
import aimrt_py_log
import aimrt_py_pb_chn

import event_pb2

class NormalPublisherModule(aimrt_py.ModuleBase):
    def __init__(self):
        super().__init__()
        self.logger = aimrt_py.LoggerRef()
        self.publisher = aimrt_py.PublisherRef()

    def Info(self):
        info = aimrt_py.ModuleInfo()
        info.name = "NormalPublisherModule"
        return info

    def Initialize(self, core):
        self.logger = core.GetLogger()
        aimrt_py_log.info(self.logger, "Module initialize")

        # get channel publisher
        self.publisher = core.GetChannelHandle().GetPublisher("test_topic")
        if (not self.publisher):
            aimrt_py_log.error(self.logger, "Get publisher for 'test_topic' failed.")
            return False

        # register msg type
        aimrt_py_pb_chn.RegisterPublishType(self.publisher, event_pb2.ExampleEventMsg)

        return True

    def Start(self):
        aimrt_py_log.info(self.logger, "Module start")

        # publish msg
        event_msg = event_pb2.ExampleEventMsg()
        event_msg.msg = "hello world"
        aimrt_py_pb_chn.Publish(self.publisher, event_msg)

        return True

    def Shutdown(self):
        aimrt_py_log.info(self.logger, "Module shutdown")
```

### Sub接口



用户如果需要订阅一个Msg，需要使用以下接口：
- `aimrt_py_pb_chn.Subscribe(subscriber, msg_type, handle)->bool` ： 用于订阅一种消息

其中handle必须要是一个能处理`msg_type`类型消息的回调方法。


注意：
- 只能在`Initialize`调用订阅接口；
- 不允许在一个`subscriber`中重复订阅同一种类型；
- 如果订阅失败，会返回false；


此外还需要注意的是，由哪个执行器来执行订阅的回调跟具体的Channel后端实现有关，这个是运行阶段通过配置才能确定的，使用者在编写逻辑代码时不应有任何假设。一般来说，如果回调中的任务非常轻量，那就可以直接在回调里处理；但如果回调中的任务比较重，那最好调度到其他专门执行任务的执行器里处理。


以下是一个示例，基于`Module`模式在`Initialize`方法中拿到`CoreRef`句柄，如果在App模式下Create Module方式拿到`CoreRef`句柄，使用方法也类似：
```python
import aimrt_py
import aimrt_py_log
import aimrt_py_pb_chn

from google.protobuf.json_format import MessageToJson
import event_pb2


class NormalSubscriberModule(aimrt_py.ModuleBase):
    def __init__(self):
        super().__init__()
        self.logger = aimrt_py.LoggerRef()
        self.subscriber = aimrt_py.SubscriberRef()

    def Info(self):
        info = aimrt_py.ModuleInfo()
        info.name = "NormalSubscriberModule"
        return info

    def Initialize(self, core):
        self.logger = core.GetLogger()
        aimrt_py_log.info(self.logger, "Module initialize")

        # get channel subscriber
        self.subscriber = core.GetChannelHandle().GetSubscriber("test_topic")
        if (not self.subscriber):
            aimrt_py_log.error(self.logger, "Get subscriber for 'test_topic' failed.")
            return False

        # define msg handle
        def EventHandle(msg):
            aimrt_py_log.info(self.logger, "Receive new pb event, data: {}".format(MessageToJson(msg)))

        # subscrib msg
        aimrt_py_pb_chn.Subscribe(self.subscriber, event_pb2.ExampleEventMsg, EventHandle)

        return True

    def Start(self):
        aimrt_py_log.info(self.logger, "Module start")

        return True

    def Shutdown(self):
        aimrt_py_log.info(self.logger, "Module shutdown")

```


## RpcHandleRef：RPC句柄

AimRT Python接口当前只支持protobuf形式的协议。AimRT提供了一个protoc插件，来生成基于protobuf的rpc桩代码。


AimRT中，模块可以通过调用`CoreRef`句柄的`GetRpcHandle()`接口，获取`RpcHandleRef`句柄。开发者在使用RPC功能时必须要按照一定的步骤，调用`RpcHandleRef`中的几个核心接口：
- Client端：
  - 在`Initialize`阶段，调用**注册RPC Client方法**的接口；
  - 在`Start`阶段，调用**RPC Invoke**的接口，以实现RPC调用；
- Server端：
  - 在`Initialize`阶段，**注册RPC Server服务**的接口；

一般情况下，使用者不会直接使用`RpcHandleRef`直接提供的那些接口，而是根据RPC IDL文件生成一些桩代码，对`RpcHandleRef`句柄进行一些封装，然后在业务代码中使用这些桩代码提供的接口。


此外，在RPC调用或者RPC处理时，使用者还可以通过一个`status`变量获取RPC请求时框架的错误情况，其中最主要的是一个错误码字段，其枚举值可以参考[rpc_status_base.h](../../../src/interface/aimrt_module_c_interface/rpc/rpc_status_base.h)文件中的定义。

### 基于protobuf生成python桩代码


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
protoc --aimrt_rpc_out=. --plugin=protoc-gen-aimrt_rpc=./protoc_plugin_py_gen_aimrt_rpc.py rpc.proto
```

这将生成`rpc_aimrt_rpc_pb2.py`文件，包含了根据定义的服务生成的Python接口。

### Client接口

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


### Server接口


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
