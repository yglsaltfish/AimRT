# Rpc


## RPC句柄概述

相关链接：
- 代码文件：
  - [aimrt_module_cpp_interface/rpc/rpc_handle.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/rpc/rpc_handle.h)
  - [aimrt_module_cpp_interface/rpc/rpc_context.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/rpc/rpc_context.h)
  - [aimrt_module_cpp_interface/rpc/rpc_status.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/rpc/rpc_status.h)
  - [aimrt_module_cpp_interface/rpc/rpc_filter.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/rpc/rpc_filter.h)
- 参考示例：
  - [protobuf_rpc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/protobuf_rpc)
  - [ros2_rpc](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/ros2_rpc)


AimRT中，模块可以通过调用`CoreRef`句柄的`GetRpcHandle()`接口，获取`aimrt::rpc::RpcHandleRef`句柄。开发者在使用RPC功能时必须要按照一定的步骤，调用几个核心接口：
- Client端：
  - 在`Initialize`阶段，调用**注册RPC Client方法**的接口；
  - 在`Start`阶段，调用**RPC Invoke**的接口，以实现RPC调用；
- Server端：
  - 在`Initialize`阶段，**注册RPC Server服务**的接口；

一般情况下，开发者不会直接使用`aimrt::rpc::RpcHandleRef`直接提供的接口，而是根据RPC IDL文件生成一些桩代码，对`RpcHandleRef`句柄做一些封装，然后在业务代码中使用这些桩代码提供的封装过的接口。

AimRT官方支持两种协议IDL：**Protobuf**和**Ros2 Srv**，并提供了针对这两种协议IDL生成桩代码的工具。生成出来的RPC接口除了协议类型不同，其他的Api风格都一致。


## 协议类型

一般来说，协议都是使用一种与具体的编程语言无关的`IDL`(Interface description language)描述，然后由某种工具转换为各个语言的代码。对于RPC来说，这里需要两个步骤：
- 参考上一节`Channel`中的介绍，开发者需要先利用一些官方的工具为协议文件中的**消息类型**生成指定编程语言中的代码；
- 开发者需要使用AimRT提供的工具，为协议文件中**服务定义**生成指定编程语言中的代码；

### Protobuf

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

然后使用Protobuf官方提供的protoc工具进行转换，生成消息结构部分的C++桩代码，例如：
```shell
protoc --cpp_out=. rpc.proto
```

这将生成`rpc.pb.h`和`rpc.pb.cc`文件，包含了根据定义的消息类型生成的C++类和方法。

在这之后，还需要使用AimRT提供的protoc插件，生成服务定义部分的C++桩代码，例如：
```shell
protoc --aimrt_rpc_out=. --plugin=protoc-gen-aimrt_rpc=./protoc_plugin_py_gen_aimrt_cpp_rpc.py rpc.proto
```

这将生成`rpc.aimrt_rpc.pb.h`和`rpc.aimrt_rpc.pb.cc`文件，包含了根据定义的服务生成的C++类和方法。

请注意，以上这套原生的代码生成方式只是为了给开发者展示底层的原理，实际使用的话需要手动处理依赖和CMake封装等方面的问题，并不推荐在项目中直接使用。开发者可以直接使用AimRT在[ProtobufAimRTRpcGenCode.cmake](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/tools/protoc_plugin_cpp_gen_aimrt_cpp_rpc/ProtobufAimRTRpcGenCode.cmake)文件中提供的CMake方法：

- `add_protobuf_aimrt_rpc_gencode_target_for_proto_files`：为一些.proto文件生成C++代码，参数如下：
  - **TARGET_NAME**：生成的CMake Target名称；
  - **PROTO_FILES**：协议文件的路径；
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

# Generate RPC service C++ code for 'rpc.proto' file. Need to rely on 'example_pb_gencode'
add_protobuf_aimrt_rpc_gencode_target_for_proto_files(
  TARGET_NAME example_rpc_aimrt_rpc_gencode
  PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto
  GENCODE_PATH ${CMAKE_CURRENT_BINARY_DIR}
  DEP_PROTO_TARGETS example_pb_gencode)
```

之后只要链接`example_rpc_aimrt_rpc_gencode`这个CMake Target即可使用该协议。例如：
```cmake
target_link_libraries(my_lib PUBLIC example_rpc_aimrt_rpc_gencode)
```

### ROS2 Srv

ROS2 Srv是一种用于在 ROS2 中进行RPC定义的格式。在使用时，开发者需要先定义一个ROS2 Package，在其中定义一个`.srv`文件，比如`example.srv`：

```
byte[]  data
---
int64   code
```

其中，以`---`来分割Req和Rsp的定义。然后直接通过ROS2提供的CMake方法`rosidl_generate_interfaces`，为Req和Rsp消息生成C++代码和CMake Target，例如：
```cmake
rosidl_generate_interfaces(example_srv_gencode
  "srv/example.srv"
)
```

之后就可以引用相关的CMake Target来使用生成的Req和Rsp消息结构C++代码。详情请参考ROS2的官方文档和AimRT提供的Example。

在生成了Req和Rsp消息结构的C++代码后，开发者还需要使用AimRT提供的Python脚本工具，生成服务定义部分的C++桩代码，例如：
```shell
python3 ARGS ./ros2_py_gen_aimrt_cpp_rpc.py --pkg_name=example_pkg --srv_file=./example.srv --output_path=./
```

这将生成`example.aimrt_rpc.srv.h`和`example.aimrt_rpc.srv.cc`文件，包含了根据定义的服务生成的C++类和方法。


请注意，以上这套原生的代码生成方式只是为了给开发者展示底层的原理，实际使用的话需要手动处理依赖和CMake封装等方面的问题，并不推荐在项目中直接使用。开发者可以直接使用AimRT在[Ros2AimRTRpcGenCode.cmake](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/tools/ros2_py_gen_aimrt_cpp_rpc/Ros2AimRTRpcGenCode.cmake)文件中提供的CMake方法：


- `add_ros2_aimrt_rpc_gencode_target_for_one_file`：为单个srv文件生成RPC服务C++代码，参数如下：
  - **TARGET_NAME**：生成的CMake Target名称；
  - **PACKAGE_NAME**：ROS2协议PKG的名称；
  - **PROTO_FILE**：协议文件的路径；
  - **GENCODE_PATH**：生成的桩代码存放路径；
  - **DEP_PROTO_TARGETS**：依赖的协议 CMake Target；
  - **OPTIONS**：传递给工具的其他参数；


使用示例如下：
```cmake
# Generate RPC service C++ code for the example '.srv' file. It is necessary to rely on the CMake Target related to ROS2 messages, which is defined in '${ROS2_EXAMPLE_CMAKE_TARGETS}'
add_ros2_aimrt_rpc_gencode_target_for_one_file(
  TARGET_NAME example_ros2_rpc_aimrt_rpc_gencode
  PACKAGE_NAME example_pkg
  PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/srv/example.srv
  GENCODE_PATH ${CMAKE_CURRENT_BINARY_DIR}
  DEP_PROTO_TARGETS
    rclcpp::rclcpp
    ${ROS2_EXAMPLE_CMAKE_TARGETS})
```

之后只要链接`example_ros2_rpc_aimrt_rpc_gencode`这个CMake Target即可使用该协议。例如：
```cmake
target_link_libraries(my_lib PUBLIC example_ros2_rpc_aimrt_rpc_gencode)
```

## Status

相关链接：
- 代码文件：
 - [aimrt_module_cpp_interface/rpc/rpc_status.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/rpc/rpc_status.h)
 - [aimrt_module_c_interface/rpc/rpc_status_base.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_c_interface/rpc/rpc_status_base.h)


在RPC调用或者RPC处理时，使用者可以通过一个`aimrt::rpc::Status`类型的变量获取RPC过程中的错误情况，其包含的接口如下：
```cpp
namespace aimrt::rpc {

class Status {
 public:
  explicit Status(aimrt_rpc_status_code_t code);
  explicit Status(uint32_t code);

  bool OK() const;

  operator bool() const;

  void SetCode(uint32_t code);
  void SetCode(aimrt_rpc_status_code_t code);

  uint32_t Code() const;

  std::string ToString() const;

  static std::string_view GetCodeMsg(uint32_t code);
};

}  // namespace aimrt::rpc
```

`Status`类型非常轻量，其中只包含一个错误码字段。使用者可以通过构造函数或Set方法设置这个code，也可以通过Get方法获取这个code。错误码的枚举值可以参考[rpc_status_base.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_c_interface/rpc/rpc_status_base.h)文件中的定义。



## Context


相关链接：
- 代码文件：[aimrt_module_cpp_interface/rpc/rpc_context.h](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/interface/aimrt_module_cpp_interface/rpc/rpc_context.h)


开发者在调用RPC时，可以传入一个`aimrt::rpc::Context`，在处理RPC时，也会得到一个`aimrt::rpc::ContextRef`。`ContextRef`类型是`Context`类型的引用，两者包含的接口基本一致，如下：

```cpp
namespace aimrt::rpc {

class Context {
 public:
  std::chrono::nanoseconds Timeout() const;
  void SetTimeout(std::chrono::nanoseconds timeout);

  std::string_view GetMetaValue(std::string_view key) const;
  void SetMetaValue(std::string_view key, std::string_view val);
  std::set<std::string_view> GetMetaKeys() const;

  std::string_view GetToAddr() const;
  void SetToAddr(std::string_view val);

  std::string_view GetSerializationType() const;
  void SetSerializationType(std::string_view val);

  std::string ToString() const;
};

class ContextRef {
 public:
  std::chrono::nanoseconds Timeout() const;
  void SetTimeout(std::chrono::nanoseconds timeout);

  std::string_view GetMetaValue(std::string_view key) const;
  void SetMetaValue(std::string_view key, std::string_view val);
  std::set<std::string_view> GetMetaKeys() const;

  std::string_view GetToAddr() const;
  void SetToAddr(std::string_view val);

  std::string_view GetSerializationType() const;
  void SetSerializationType(std::string_view val);

  std::string ToString() const;
};

}  // namespace aimrt::rpc
```

`Context`主要是传入一些特殊的信息给rpc后端，比如超时设置、目标地址等，因此对其具体的处理行为请参考不同rpc后端的文档。

## Client接口

### Client接口概述

在AimRT RPC桩代码工具生成的代码里，如`rpc.aimrt_rpc.pb.h`或者`example.aimrt_rpc.srv.h`文件里，提供了四种类型的Client Proxy接口，开发者基于这些Proxy接口类来发起RPC调用：
- **同步型接口**：名称一般为`XXXSyncProxy`；
- **异步回调型接口**：名称一般为`XXXAsyncProxy`；
- **异步Future型接口**：名称一般为`XXXFutureProxy`；
- **无栈协程型接口**：名称一般为`XXXCoProxy`；

这四种Proxy类型可以混合使用，开发者可以根据自身需求选用。它们除了实际调用RPC时的接口不一样，一些公有的接口都是一致的，如下：
```cpp
class XXXProxy {
 public:
  explicit ProxyBase(aimrt::rpc::RpcHandleRef rpc_handle_ref);

  std::shared_ptr<Context> NewContextSharedPtr() const;

  void SetDefaultContextSharedPtr(const std::shared_ptr<Context>& ctx_ptr);

  std::shared_ptr<Context> GetDefaultContextSharedPtr() const;
};
```

这些公用接口的说明如下：
- 所有类型的Proxy都需要从`aimrt::rpc::RpcHandleRef`句柄构造；
- 所有类型的Proxy都可以设置一个默认Context：
  - 如果在调用RPC时未传入Context/传入了空的Context，则会使用该Proxy默认的Context；
  - 使用者可以通过`SetDefaultContextSharedPtr`和`GetDefaultContextSharedPtr`方法来设置、获取默认Context的智能指针；
  - 使用者可以通过`NewContextSharedPtr`方法从默认Context复制得到一份新的Context的智能指针；


### 同步型接口

参考示例：
- [protobuf_rpc_sync_client](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/protobuf_rpc/module/normal_rpc_sync_client_module/normal_rpc_sync_client_module.cc)
- [ros2_rpc_sync_client](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/ros2_rpc/module/normal_rpc_sync_client_module/normal_rpc_sync_client_module.cc)


同步型接口在使用上最简单，但在运行效率上是最低的。它通过阻塞当前线程，等待RPC接口返回。一般可以在一些不要求性能的场合为了提高开发效率而使用这种方式，但不推荐在高性能要求的场景使用。

使用同步型接口发起RPC调用非常简单，一般分为以下几个步骤：
- **Step 0**：引用桩代码头文件，例如`xxx.aimrt_rpc.pb.h`或者`xxx.aimrt_rpc.srv.h`，其中有同步接口的句柄`XXXSyncProxy`；
- **Step 1**：在`Initialize`阶段调用`RegisterClientFunc`方法注册RPC Client；
- **Step 2**：在`Start`阶段里某个业务函数里发起RPC调用：
  - **Step 2-1**：创建一个`XXXSyncProxy`，构造参数是`aimrt::rpc::RpcHandleRef`类型句柄。proxy非常轻量，可以随用随创建；
  - **Step 2-2**：创建Req、Rsp，并填充Req内容；
  - **Step 2-3**：【可选】创建ctx，设置超时等信息；
  - **Step 2-4**：基于proxy，传入ctx、Req、Rsp，发起RPC调用，同步等待RPC调用结束，获取返回的status；
  - **Step 2-5**：解析status和Rsp；


以下是一个简单的基于protobuf的示例，基于ROS2 Srv的语法也基本类似：
```cpp
#include "rpc.aimrt_rpc.pb.h"

bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  // Step 1: RegisterClientFunc
  bool ret = aimrt::protocols::example::RegisterExampleServiceClientFunc(core_.GetRpcHandle());
  AIMRT_CHECK_ERROR_THROW(ret, "Register client failed.");

  return true;
}

// Step 2: Call rpc
void HelloWorldModule::Foo() {
  // Step 2-1: Create a proxy
  ExampleServiceSyncProxy proxy(core_.GetRpcHandle());

  // Step 2-2: Create req and rsp
  ExampleReq req;
  ExampleRsp rsp;
  req.set_msg("hello world");

  // Step 2-3: Create context
  auto ctx = proxy.NewContextSharedPtr();
  ctx->SetTimeout(std::chrono::seconds(3));

  // Step 2-4: Call rpc
  auto status = proxy.ExampleFunc(ctx, req, rsp);

  // Step 2-5: Parse rsp
  if (status.OK()) {
    auto msg = rsp.msg();
    // ...
  } else {
    // ...
  }
}
```


### 异步回调型接口

参考示例：
- [protobuf_rpc_async_client](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/protobuf_rpc/module/normal_rpc_async_client_module/normal_rpc_async_client_module.cc)
- [ros2_rpc_async_client](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/ros2_rpc/module/normal_rpc_async_client_module/normal_rpc_async_client_module.cc)


异步回调型接口使用回调来返回异步结果，在性能上表现最好，但开发友好度是最低的，很容易陷入回调地狱。

使用异步回调型接口发起RPC调用一般分为以下几个步骤：
- **Step 0**：引用桩代码头文件，例如`xxx.aimrt_rpc.pb.h`或者`xxx.aimrt_rpc.srv.h`，其中有异步接口的句柄`XXXAsyncProxy`；
- **Step 1**：在`Initialize`阶段调用`RegisterClientFunc`方法注册RPC Client；
- **Step 2**：在`Start`阶段里某个业务函数里发起RPC调用：
  - **Step 2-1**：创建一个`XXXAsyncProxy`，构造参数是`aimrt::rpc::RpcHandleRef`。proxy非常轻量，可以随用随创建；
  - **Step 2-2**：创建Req、Rsp，并填充Req内容；
  - **Step 2-3**：【可选】创建ctx，设置超时等信息；
  - **Step 2-4**：基于proxy，传入ctx、Req、Rsp和结果回调，发起RPC调用，并保证在整个调用周期里ctx、Req、Rsp都保持生存；
  - **Step 2-5**：在回调函数中获取返回的status，解析status和Rsp；

前几个步骤与同步型接口基本一致，区别在于**Step 2-4**需要使用异步回调的方式来获取结果。以下是一个简单的基于protobuf的示例，基于ROS2 Srv的语法也基本类似：
```cpp
#include "rpc.aimrt_rpc.pb.h"

bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  // Step 1: RegisterClientFunc
  bool ret = aimrt::protocols::example::RegisterExampleServiceClientFunc(core_.GetRpcHandle());
  AIMRT_CHECK_ERROR_THROW(ret, "Register client failed.");

  return true;
}

// Step 2: Call rpc
void HelloWorldModule::Foo() {
  // Step 2-1: Create a proxy
  ExampleServiceAsyncProxy proxy(core_.GetRpcHandle());

  // Step 2-2: Create req and rsp
  // To ensure that the lifecycle of req and rsp is longer than RPC calls, we should use smart pointers here
  auto req = std::make_shared<ExampleReq>();
  auto rsp = std::make_shared<ExampleRsp>();
  req->set_msg("hello world");

  // Step 2-3: Create context
  // To ensure that the lifecycle of context is longer than RPC calls, we should use smart pointers here
  auto ctx = proxy.NewContextSharedPtr();
  ctx->SetTimeout(std::chrono::seconds(3));

  // Step 2-4: Call rpc with callback
  proxy.GetBarData(
      ctx, *req, *rsp,
      [this, ctx, req, rsp](aimrt::rpc::Status status) {
        // Step 2-5: Parse rsp
        if (status.OK()) {
          auto msg = rsp->msg();
          // ...
        } else {
          // ...
        }
      });
}
```

### 异步Future型接口

参考示例：
- [protobuf_rpc_future_client](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/protobuf_rpc/module/normal_rpc_future_client_module/normal_rpc_future_client_module.cc)
- [ros2_rpc_future_client](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/ros2_rpc/module/normal_rpc_future_client_module/normal_rpc_future_client_module.cc)


异步Future型接口基于`std::future`来返回异步结果，开发者可以在发起RPC调用后先去做其他事情，等需要RPC结果时在调用`std::future::get`方法来阻塞的获取结果。它在一定程度上兼顾了性能和开发友好度，属于同步型和异步回调型中间的一个选择。


使用异步Future型接口发起RPC调用一般分为以下几个步骤：
- **Step 0**：引用桩代码头文件，例如`xxx.aimrt_rpc.pb.h`或者`xxx.aimrt_rpc.srv.h`，其中有异步接口的句柄`XXXFutureProxy`；
- **Step 1**：在`Initialize`阶段调用`RegisterClientFunc`方法注册RPC Client；
- **Step 2**：在`Start`阶段里某个业务函数里发起RPC调用：
  - **Step 2-1**：创建一个`XXXFutureProxy`，构造参数是`aimrt::rpc::RpcHandleRef`。proxy非常轻量，可以随用随创建；
  - **Step 2-2**：创建Req、Rsp，并填充Req内容；
  - **Step 2-3**：【可选】创建ctx，设置超时等信息；
  - **Step 2-4**：基于proxy，传入ctx、Req、Rsp和结果回调，发起RPC调用，获取一个`std::future<Status>`句柄；
  - **Step 2-5**：在后续某个时间，阻塞的调用`std::future<Status>`句柄的`get()`方法，获取status值，并解析status和Rsp。需要保证在整个调用周期里ctx、Req、Rsp都保持生存；



以下是一个简单的基于protobuf的示例，基于ROS2 Srv的语法也基本类似：
```cpp
#include "rpc.aimrt_rpc.pb.h"

bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  // Step 1: RegisterClientFunc
  bool ret = aimrt::protocols::example::RegisterExampleServiceClientFunc(core_.GetRpcHandle());
  AIMRT_CHECK_ERROR_THROW(ret, "Register client failed.");

  return true;
}

// Step 2: Call rpc
void HelloWorldModule::Foo() {
  // Step 2-1: Create a proxy
  ExampleServiceFutureProxy proxy(core_.GetRpcHandle());

  // Step 2-2: Create req and rsp
  ExampleReq req;
  ExampleRsp rsp;
  req.set_msg("hello world");

  // Step 2-3: Create context
  auto ctx = proxy.NewContextSharedPtr();
  ctx->SetTimeout(std::chrono::seconds(3));

  // Step 2-4: Call rpc, return 'std::future<Status>' 
  auto status_future = proxy.ExampleFunc(ctx, req, rsp);

  // ...

  // Step 2-5: Call 'get()' method of 'status_future', Parse rsp
  auto status = status_future.get();
  if (status.OK()) {
    auto msg = rsp.msg();
    // ...
  } else {
    // ...
  }
}
```


### 无栈协程型接口

参考示例：
- [protobuf_rpc_co_client](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/protobuf_rpc/module/normal_rpc_co_client_module/normal_rpc_co_client_module.cc)
- [ros2_rpc_co_client](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/ros2_rpc/module/normal_rpc_co_client_module/normal_rpc_co_client_module.cc)


AimRT为RPC Client端提供了一套基于C++20协程和[C++ executors提案](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html)当前的一个实现库[libunifex](https://github.com/facebookexperimental/libunifex)来实现的一套无栈协程形式的接口。无栈协程接口在本质上是对异步回调型接口的封装，在性能上基本与异步回调型接口一致，但大大提升了开发友好度。

使用协程型接口发起RPC调用一般分为以下几个步骤：
- **Step 0**：引用桩代码头文件，例如`xxx.aimrt_rpc.pb.h`或者`xxx.aimrt_rpc.srv.h`，其中有协程接口的句柄`XXXCoProxy`；
- **Step 1**：在`Initialize`阶段调用`RegisterClientFunc`方法注册RPC Client；
- **Step 2**：在`Start`阶段里某个业务协程里发起RPC调用：
  - **Step 2-1**：创建一个`XXXCoProxy`，构造参数是`aimrt::rpc::RpcHandleRef`。proxy非常轻量，可以随用随创建；
  - **Step 2-2**：创建Req、Rsp，并填充Req内容；
  - **Step 2-3**：【可选】创建ctx，设置超时等信息；
  - **Step 2-4**：基于proxy，传入ctx、Req、Rsp和结果回调，发起RPC调用，在协程中等待RPC调用结束，获取返回的status；
  - **Step 2-5**：解析status和Rsp；


整个接口风格与同步型接口几乎一样，但必须要在协程中调用。以下是一个简单的基于protobuf的示例，基于ROS2 Srv的语法也基本类似：
```cpp
#include "rpc.aimrt_rpc.pb.h"

bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  // Step 1: RegisterClientFunc
  bool ret = aimrt::protocols::example::RegisterExampleServiceClientFunc(core_.GetRpcHandle());
  AIMRT_CHECK_ERROR_THROW(ret, "Register client failed.");

  return true;
}

// Step 2: Call rpc
co::Task<void> HelloWorldModule::Foo() {
  // Step 2-1: Create a proxy
  ExampleServiceCoProxy proxy(core_.GetRpcHandle());

  // Step 2-2: Create req and rsp
  ExampleReq req;
  ExampleRsp rsp;
  req.set_msg("hello world");

  // Step 2-3: Create context
  auto ctx = proxy.NewContextSharedPtr();
  ctx->SetTimeout(std::chrono::seconds(3));

  // Step 2-4: Call rpc
  auto status = co_await proxy.ExampleFunc(ctx, req, rsp);

  // Step 2-5: Parse rsp
  if (status.OK()) {
    auto msg = rsp.msg();
    // ...
  } else {
    // ...
  }
}
```

## Server接口

### Server接口概述

在AimRT RPC桩代码工具生成的代码里，如`rpc.aimrt_rpc.pb.h`或者`example.aimrt_rpc.srv.h`里，提供了三种类型的Service基类，开发者继承这些Service基类，实现其中的虚接口来提供实际的RPC服务：
- **同步型接口**：名称一般为`XXXSyncService`；
- **异步回调型接口**：名称一般为`XXXAsyncService`；
- **无栈协程型接口**：名称一般为`XXXCoService`；

在单个service内，这三种类型的不能混合使用，只能选择一种，开发者可以根据自身需求选用。

### 同步型接口

参考示例：
- [protobuf_rpc_sync_service](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/protobuf_rpc/module/normal_rpc_sync_server_module/service.cc)
- [ros2_rpc_sync_service](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/ros2_rpc/module/normal_rpc_sync_server_module/service.cc)


同步型接口在使用上最简单，但很多时候实现的service中需要请求下游，会有一些异步调用，这种情况下只能阻塞的等待下游调用完成，可能会造成运行效率上的降低。一般可以在处理一些简单的请求、不需要发起其他异步调用的场景下使用同步型接口。


使用同步型接口实现RPC服务，一般分为以下几个步骤：
- **Step 0**：引用桩代码头文件，例如`xxx.aimrt_rpc.pb.h`或者`xxx.aimrt_rpc.srv.h`，其中有同步接口的Service基类`XXXSyncService`；
- **Step 1**：开发者实现一个Impl类，继承`XXXSyncService`，并实现其中的虚接口；
  - **Step 1-1**：解析Req，并填充Rsp；
  - **Step 1-2**：返回`Status`；
- **Step 2**：在`Initialize`阶段调用`RegisterService`方法注册RPC Service；


以下是一个简单的基于protobuf的示例，基于ROS2 Srv的语法也基本类似：
```cpp
#include "rpc.aimrt_rpc.pb.h"

// Step 1: Implement an Impl class that inherits 'XXXSyncService'
class ExampleServiceSyncServiceImpl : public ExampleServiceSyncService {
 public:
  ExampleServiceSyncServiceImpl() = default;
  ~ExampleServiceSyncServiceImpl() override = default;

  aimrt::rpc::Status ExampleFunc(
      aimrt::rpc::ContextRef ctx, const ExampleReq& req, ExampleRsp& rsp) override {
    // Step 1-1: Parse req and set rsp
    rsp.set_msg("echo " + req.msg());

    // Step 1-2: Return status
    return aimrt::rpc::Status();
  }
};

bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  // Step 2: Register rpc service
  service_ptr_ = std::make_shared<ExampleServiceSyncServiceImpl>();

  bool ret = core_.GetRpcHandle().RegisterService(service_ptr_.get());
  AIMRT_CHECK_ERROR_THROW(ret, "Register service failed.");

  return true;
}
```

### 异步回调型接口

参考示例：
- [protobuf_rpc_async_service](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/protobuf_rpc/module/normal_rpc_async_server_module/service.cc)
- [ros2_rpc_async_service](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/ros2_rpc/module/normal_rpc_async_server_module/service.cc)


异步回调型接口会传递一个回调给开发者，开发者在RPC处理完成后调用这个回调来传递最终处理结果。这种方式可以在RPC中发起其他异步调用，由于不会阻塞，因此性能表现通常最好，但通常会导致开发出的代码难以阅读和维护。


使用异步回调型接口实现RPC服务，一般分为以下几个步骤：
- **Step 0**：引用桩代码头文件，例如`xxx.aimrt_rpc.pb.h`或者`xxx.aimrt_rpc.srv.h`，其中有异步接口的Service基类`XXXAsyncService`；
- **Step 1**：开发者实现一个Impl类，继承`XXXAsyncService`，并实现其中的虚接口；
  - **Step 1-1**：解析Req，并填充Rsp；
  - **Step 1-2**：调用callback将`Status`传递回去；
- **Step 2**：在`Initialize`阶段调用`RegisterService`方法注册RPC Service；


以下是一个简单的基于protobuf的示例，基于ROS2 Srv的语法也基本类似：
```cpp
// Step 1: Implement an Impl class that inherits 'XXXAsyncService'
class ExampleServiceAsyncServiceImpl : public ExampleServiceAsyncService {
 public:
  ExampleServiceAsyncServiceImpl() = default;
  ~ExampleServiceAsyncServiceImpl() override = default;

  void ExampleFunc(
      aimrt::rpc::ContextRef ctx, const ExampleReq& req, ExampleRsp& rsp,
      std::function<void(aimrt::rpc::Status)>&& callback) override {
    // Step 1-1: Parse req and set rsp
    rsp.set_msg("echo " + req.msg());

    // Step 1-2: Return status by callback
    callback(aimrt::rpc::Status());
  }
};

bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  // Step 2: Register rpc service
  service_ptr_ = std::make_shared<ExampleServiceAsyncServiceImpl>();

  bool ret = core_.GetRpcHandle().RegisterService(service_ptr_.get());
  AIMRT_CHECK_ERROR_THROW(ret, "Register service failed.");

  return true;
}
```


### 无栈协程型接口

参考示例：
- [protobuf_rpc_co_service](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/protobuf_rpc/module/normal_rpc_co_server_module/service.cc)
- [ros2_rpc_co_service](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/examples/cpp/ros2_rpc/module/normal_rpc_co_server_module/service.cc)


与RPC Client端一样，在RPC Service端，AimRT也提供了一套基于C++20协程和[C++ executors提案](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html)当前的一个实现库[libunifex](https://github.com/facebookexperimental/libunifex)来实现的一套无栈协程形式的接口。无栈协程接口在本质上是对异步回调型接口的封装，在性能上基本与异步回调型接口一致，但大大提升了开发友好度。


使用协程型接口实现RPC服务，一般分为以下几个步骤：
- **Step 0**：引用桩代码头文件，例如`xxx.aimrt_rpc.pb.h`或者`xxx.aimrt_rpc.srv.h`，其中有异步接口的Service基类`XXXCoService`；
- **Step 1**：开发者实现一个Impl类，继承`XXXCoService`，并实现其中的虚接口；
  - **Step 1-1**：解析Req，并填充Rsp；
  - **Step 1-2**：使用co_return返回`Status`；
- **Step 2**：在`Initialize`阶段调用`RegisterService`方法注册RPC Service；


整个接口风格与同步型接口几乎一样。以下是一个简单的基于protobuf的示例，基于ROS2 Srv的语法也基本类似：
```cpp
// Step 1: Implement an Impl class that inherits 'XXXCoService'
class ExampleServiceCoServiceImpl : public ExampleServiceCoService {
 public:
  ExampleServiceCoServiceImpl() = default;
  ~ExampleServiceCoServiceImpl() override = default;

  co::Task<aimrt::rpc::Status> ExampleFunc(
      aimrt::rpc::ContextRef ctx, const ExampleReq& req, ExampleRsp& rsp) override {
    // Step 1-1: Parse req and set rsp
    rsp.set_msg("echo " + req.msg());

    // Step 1-2: Return status by co_return
    co_return aimrt::rpc::Status();
  }
};

bool HelloWorldModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  // Step 2: Register rpc service
  service_ptr_ = std::make_shared<ExampleServiceCoServiceImpl>();

  bool ret = core_.GetRpcHandle().RegisterService(service_ptr_.get());
  AIMRT_CHECK_ERROR_THROW(ret, "Register service failed.");

  return true;
}
```
