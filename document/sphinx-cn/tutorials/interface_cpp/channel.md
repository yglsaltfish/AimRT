# Channel

## Channel句柄概述

相关链接：
- 代码文件：
  - [aimrt_module_cpp_interface/channel/channel_context.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/channel/channel_context.h)
  - [aimrt_module_cpp_interface/channel/channel_handle.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/channel/channel_handle.h)
- Protobuf Channel（需CMake引用**aimrt::interface::aimrt_module_protobuf_interface**）：
  - [aimrt_module_protobuf_interface/channel/protobuf_channel.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_protobuf_interface/channel/protobuf_channel.h)
- Ros2 Channel（需CMake引用**aimrt::interface::aimrt_module_ros2_interface**）：
  - [aimrt_module_ros2_interface/channel/ros2_channel.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_ros2_interface/channel/ros2_channel.h)
- 参考示例：
  - [protobuf_channel](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/protobuf_channel)
  - [ros2_channel](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/ros2_channel)

AimRT中，模块可以通过调用`CoreRef`句柄的`GetChannelHandle()`接口，获取`aimrt::channel::ChannelHandleRef`句柄，来使用Channel功能。其提供的核心接口如下：
```cpp
namespace aimrt::channel {

class ChannelHandleRef {
 public:
  PublisherRef GetPublisher(std::string_view topic) const;

  SubscriberRef GetSubscriber(std::string_view topic) const;
};

}  // namespace aimrt::channel
```

使用者可以调用`ChannelHandleRef`中的`GetPublisher`方法和`GetSubscriber`方法，获取指定Topic名称的`aimrt::channel::PublisherRef`句柄和`aimrt::channel::SubscriberRef`句柄，分别用于Channel发布和订阅。这两个方法使用注意如下：
  - 这两个接口是线程安全的。
  - 这两个接口可以在`Initialize`阶段和`Start`阶段使用。


`PublisherRef`和`SubscriberRef`句柄提供了一个与具体协议类型无关的Api接口，但除非开发者想要使用自定义的消息类型，才需要直接调用它们提供的接口。

AimRT官方支持了两种协议类型：**Protobuf**和**Ros2 Message**，并提供了这两种协议类型的Channel接口封装。这两套Channel接口除了协议类型不同，其他的Api风格都一致，开发者一般直接使用这两套与协议类型绑定的Channel接口即可。


## 消息类型

一般来说，协议都是使用一种与具体的编程语言无关的`IDL`(Interface description language)描述，然后由某种工具转换为各个语言的代码。此处简要介绍一下几种`IDL`如何转换为Cpp代码，进阶的使用方式请参考对应的官方文档。

### Protobuf

[Protobuf](https://protobuf.dev/)是一种由Google开发的、用于序列化结构化数据的轻量级、高效的数据交换格式，是一种广泛使用的IDL。它类似于XML和JSON，但更为紧凑、快速、简单，且可扩展性强。

在使用时，开发者需要先定义一个`.proto`文件，在其中定义一个消息结构。例如`example.proto`：

```protobuf
syntax = "proto3";

message ExampleMsg {
  string msg = 1;
  int32 num = 2;
}
```

然后使用Protobuf官方提供的protoc工具进行转换，生成C++桩代码，例如：
```shell
protoc --cpp_out=. example.proto
```

这将生成`example.pb.h`和`example.pb.cc`文件，包含了根据定义的消息类型生成的C++类和方法。

请注意，以上这套原生的代码生成方式只是为了给开发者展示底层的原理，实际使用时还需要手动处理依赖和CMake封装等方面的问题，因此并不推荐在项目中直接使用。开发者可以直接使用AimRT在[ProtobufGenCode.cmake](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/cmake/ProtobufGenCode.cmake)文件中提供的两个CMake方法：
- `add_protobuf_gencode_target_for_proto_path`：为某个路径下的.proto文件生成C++代码，参数如下：
  - **TARGET_NAME**：生成的CMake Target名称；
  - **PROTO_PATH**：协议存放目录；
  - **GENCODE_PATH**：生成的桩代码存放路径；
  - **DEP_PROTO_TARGETS**：依赖的Proto CMake Target；
  - **OPTIONS**：传递给protoc的其他参数；
- `add_protobuf_gencode_target_for_one_proto_file`：为单个.proto文件生成C++代码；
  - **TARGET_NAME**：生成的CMake Target名称；
  - **PROTO_FILE**：单个协议文件的路径；
  - **GENCODE_PATH**：生成的桩代码存放路径；
  - **DEP_PROTO_TARGETS**：依赖的Proto CMake Target；
  - **OPTIONS**：传递给protoc的其他参数；


使用示例如下：
```cmake
# Generate C++ code for all '.proto' files in the current folder
add_protobuf_gencode_target_for_proto_path(
  TARGET_NAME example_pb_gencode
  PROTO_PATH ${CMAKE_CURRENT_SOURCE_DIR}
  GENCODE_PATH ${CMAKE_CURRENT_BINARY_DIR})
```

之后只要链接`example_pb_gencode`这个CMake Target即可使用该协议。例如：
```cmake
target_link_libraries(my_lib PUBLIC example_pb_gencode)
```

### ROS2 Message

ROS2 Message是一种用于在 ROS2 中进行通信和数据交换的结构化数据格式。在使用时，开发者需要先定义一个ROS2 Package，在其中定义一个`.msg`文件，比如`example.msg`：

```
int32   num
float32 num2
char    data
```

然后直接通过ROS2提供的CMake方法`rosidl_generate_interfaces`，为消息生成C++代码和CMake Target，例如：
```cmake
rosidl_generate_interfaces(example_msg_gencode
  "msg/example.msg"
)
```

在这之后就可以引用相关的CMake Target来使用生成的C++代码。详情请参考ROS2的官方文档和AimRT提供的Example。


## Context

相关链接：
- 代码文件：[aimrt_module_cpp_interface/channel/channel_context.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/channel/channel_context.h)

开发者在发布Channel消息时，可以传入一个`aimrt::channel::Context`，在订阅Channel消息时，也可以选择向回调中传入一个`aimrt::channel::ContextRef`。`ContextRef`类型是`Context`类型的引用，两者包含的接口基本一致，如下：

```cpp
namespace aimrt::channel {

class Context {
 public:
  std::chrono::system_clock::time_point GetMsgTimestamp() const;
  void SetMsgTimestamp(std::chrono::system_clock::time_point deadline);

  std::string_view GetMetaValue(std::string_view key) const;
  void SetMetaValue(std::string_view key, std::string_view val);
  std::vector<std::string_view> GetMetaKeys() const;

  std::string_view GetSerializationType() const;
  void SetSerializationType(std::string_view val);

  std::string ToString() const;
};

class ContextRef {
 public:
  std::chrono::system_clock::time_point GetMsgTimestamp() const;
  void SetMsgTimestamp(std::chrono::system_clock::time_point deadline);

  std::string_view GetMetaValue(std::string_view key) const;
  void SetMetaValue(std::string_view key, std::string_view val);
  std::vector<std::string_view> GetMetaKeys() const;

  std::string_view GetSerializationType() const;
  void SetSerializationType(std::string_view val);

  std::string ToString() const;
};

}  // namespace aimrt::channel
```

`Context`主要是传入一些特殊的信息给Channel后端，因此对其具体的处理行为请参考不同Channel后端的文档。

## 发布接口

相关链接：
- 参考示例：
  - protobuf_channel:[normal_publisher_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/protobuf_channel/module/normal_publisher_module/normal_publisher_module.cc)
  - ros2_channel:[normal_publisher_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/ros2_channel/module/normal_publisher_module/normal_publisher_module.cc)


AimRT提供了两种风格的接口来发布一个消息：
- 函数风格接口：
```cpp
namespace aimrt::channel {

template <typename MsgType>
bool RegisterPublishType(PublisherRef publisher);

template <typename MsgType>
void Publish(PublisherRef publisher, aimrt::channel::ContextRef ctx_ref, const MsgType& msg);

template <typename MsgType>
void Publish(PublisherRef publisher, const MsgType& msg);

}  // namespace aimrt::channel
```

- Proxy类风格接口：
```cpp
namespace aimrt::channel {

template <typename MsgType>
class PublisherProxy {
 public:
  explicit PublisherProxy(PublisherRef publisher);
  ~PublisherProxy();

  // Hook
  using HookFunc = std::function<void(std::string_view, ContextRef, const void*)>;
  template <typename... Args>
    requires std::constructible_from<HookFunc, Args...>
  void RegisterHook(Args&&... args);

  // Context
  std::shared_ptr<Context> NewContextSharedPtr() const;
  void SetDefaultContextSharedPtr(const std::shared_ptr<Context>& ctx_ptr);
  std::shared_ptr<Context> GetDefaultContextSharedPtr() const;

  // Register type
  static bool RegisterPublishType(PublisherRef publisher);
  bool RegisterPublishType() const;

  // Publish
  void Publish(ContextRef ctx_ref, const MsgType& msg) const;
  void Publish(const MsgType& msg) const;
};

}  // namespace aimrt::channel
```

Proxy类型接口可以绑定类型信息和一个默认Context，还能设置Hook方法，功能更齐全一些。但两种风格接口的基本使用效果是一致的，用户需要两个步骤来实现逻辑层面的消息发布：
- Step1：使用`RegisterPublishType`方法注册协议类型；
  - 只能在`Initialize`阶段注册；
  - 不允许在一个`PublisherRef`中重复注册同一种类型；
  - 如果注册失败，会返回false；
- Step2：使用`Publish`方法发布数据；
  - 只能在`Start`阶段之后发布数据；
  - 有两种`Publish`接口，其中一种多一个CTX参数，用于向后端、下游传递一些额外信息。CTX的具体功能由Channel后端决定。


用户`Publish`一个消息后，特定的Channel后端将处理具体的消息发布请求。此时根据不同后端的实现，有可能会阻塞一段时间，因此`Publish`方法耗费的时间是未定义的。但一般来说，Channel后端都不会阻塞`Publish`方法太久，详细信息请参考对应后端的文档。

## 订阅接口

相关链接：
- 参考示例：
  - protobuf_channel:[normal_subscriber_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/protobuf_channel/module/normal_subscriber_module/normal_subscriber_module.cc)
  - ros2_channel:[normal_subscriber_module.cc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/ros2_channel/module/normal_subscriber_module/normal_subscriber_module.cc)


AimRT提供了**函数风格**和**Proxy风格**两种风格类型的接口来订阅一个消息，同时还提供了**智能指针形式**和**协程形式**两种回调函数：
- 函数风格接口：
```cpp
// Callback accept a CTX and a smart pointer as parameters
template <MsgType>
bool Subscribe(
    SubscriberRef subscriber,
    std::function<void(ContextRef, const std::shared_ptr<const MsgType>&)>&& callback);

// Callback accept a pointer as a parameter
template <MsgType>
bool Subscribe(
    SubscriberRef subscriber,
    std::function<void(const std::shared_ptr<const MsgType>&)>&& callback);

// Coroutine callback, accept a CTX and a const reference to message as parameters
template <MsgType>
bool SubscribeCo(
    SubscriberRef subscriber,
    std::function<co::Task<void>(ContextRef, const MsgType&)>&& callback);

// Coroutine callback, accept a const reference to message as a parameter
template <MsgType>
bool SubscribeCo(
    SubscriberRef subscriber,
    std::function<co::Task<void>(const MsgType&)>&& callback);
```

- Proxy类风格接口：
```cpp
namespace aimrt::channel {

template <typename MsgType>
class SubscriberProxy {
 public:
  explicit SubscriberProxy(SubscriberRef subscriber);
  ~SubscriberProxy();

  // Hook
  using HookFunc = std::function<void(std::string_view, ContextRef, const void*)>;
  template <typename... Args>
    requires std::constructible_from<HookFunc, Args...>
  void RegisterHook(Args&&... args);

  // Subscribe
  bool Subscribe(
      std::function<void(ContextRef, const std::shared_ptr<const MsgType>&)>&& callback) const;

  bool Subscribe(
      std::function<void(const std::shared_ptr<const MsgType>&)>&& callback) const;

  bool SubscribeCo(
      std::function<co::Task<void>(ContextRef, const MsgType&)>&& callback) const;

  bool SubscribeCo(std::function<co::Task<void>(const MsgType&)>&& callback) const;
};

}  // namespace aimrt::channel
```

Proxy类型接口可以绑定类型信息，还能设置Hook方法，功能更齐全一些。使用Subscribe接口时需要注意：
- 只能在`Initialize`调用订阅接口；
- 不允许在一个`SubscriberRef`中重复订阅同一种类型；
- 如果订阅失败，会返回false；


此外还需要注意的是，由哪个执行器来执行订阅的callback，这和具体的Channel后端实现有关，在运行阶段通过配置才能确定，使用者在编写逻辑代码时不应有任何假设。详细信息请参考对应后端的文档。

一般来说，如果回调中的任务非常轻量，那就可以直接在回调里处理；但如果回调中的任务比较重，那最好调度到其他专门执行任务的执行器里处理。

