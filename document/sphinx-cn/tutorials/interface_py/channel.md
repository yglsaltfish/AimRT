# Channel

## Channel句柄概述

AimRT Python接口当前只支持protobuf形式的协议。AimRT提供了`aimrt_py_pb_chn`包，来提供基于protobuf的channel功能。

AimRT中，模块可以通过调用`CoreRef`句柄的`GetChannelHandle()`接口，获取`ChannelHandleRef`句柄，来使用Channel功能。其提供的核心接口如下：
- `GetPublisher(str)->PublisherRef`
- `GetSubscriber(str)->SubscriberRef`


使用者可以调用`ChannelHandleRef`中的`GetPublisher`方法和`GetSubscriber`方法，获取**指定Topic名称**的`PublisherRef`句柄和`SubscriberRef`句柄，分别用于Channel发布和订阅。关于这两个方法，有一些使用时的须知：
  - 这两个接口是线程安全的。
  - 这两个接口可以在`Initialize`阶段和`Start`阶段使用。


## 基于protobuf生成python桩代码

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


## Pub接口

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

## Sub接口



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

