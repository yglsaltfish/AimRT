
# CPP模块接口



## 业务模块生命周期

&emsp;&emsp;业务模块开发主要围绕模块的三个方法进行：
- `bool Initialize(aimrt::CoreRef core)`
- `bool Start()`
- `void Shutdown()`

&emsp;&emsp;框架会在初始化、启动、停止时调用所有模块的这三个方法，流程如下图所示：

（TODO）


&emsp;&emsp;其中，在初始化时，框架会给模块传入一个`aimrt::CoreRef`变量。该变量是一个引用，拷贝传递不会有较大开销。模块通过该变量的一些接口调用框架的功能。


## 使用配置功能

&emsp;&emsp;模块可以通过在初始化时传入的`aimrt::CoreRef`变量的`GetConfigurator()`接口，获取`aimrt::ConfiguratorRef`变量，并通过其`GetConfigFilePath()`方法获取模块配置文件的路径，模块可以依据此在初始化方法中自行读取配置文件：
```cpp
bool HelloWorldModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  try {
    aimrt::ConfiguratorRef configurator = core_.GetConfigurator();
    if (configurator) {
      // 根据返回的配置文件路径，使用相应方式读取模块配置文件。例如这里使用yaml-cpp打开该配置文件
      YAML::Node cfg_node = YAML::LoadFile(configurator.GetConfigFilePath());

      // ...
    }
  }

  return true;
}
```

&emsp;&emsp;注意：如果在框架根配置yaml文件的模块配置中指定了模块配置文件路径，则此处返回指定的文件路径。例如：
```yaml
aimrt:
  module: # 模块配置
    modules: # 模块
      - name: HelloWorldModule # 模块Name接口返回的名称
        cfg_file_path: ./cfg/my_module_cfg.txt
```
&emsp;&emsp;则此时`GetConfigFilePath()`方法返回值为`./cfg/my_module_cfg.txt`。该配置文件为用户自定义，不限制格式。

&emsp;&emsp;如果在框架根配置yaml文件中存在以模块名为名称的节点，则框架会为该模块生成一个临时yaml配置文件，并将模块配置写入到此临时文件中，此时`GetConfigFilePath()`方法返回值为这个临时配置文件的路径。例如框架的根配置yaml文件如下所示时：
```yaml
aimrt:
  configurator:
    temp_cfg_path: ./cfg/tmp # 生成的临时模块配置文件存放路径
  module: # 模块配置
    modules: # 模块
      - name: HelloWorldModule # 模块Name接口返回的名称

# 模块自定义配置，框架会为每个模块生成临时配置文件，开发者通过Configurator接口获取该配置文件路径
HelloWorldModule:
  key1: val1
  key2: val2
```
&emsp;&emsp;则此时`GetConfigFilePath()`方法将返回一个临时配置文件路径，该临时配置文件将位于`aimrt.configurator.temp_cfg_path`节点所配置的目录下，其中的内容如下：
```yaml
key1: val1
key2: val2
```

## 使用日志功能


&emsp;&emsp;模块可以使用初始化时传入的`aimrt::CoreRef`变量的`GetLogger()`方法获取日志句柄来打日志，不同模块拿到的日志句柄会根据配置有不同的表现。可以配合一些日志宏，使用C++20 format语法打日志：
```cpp
std::string s = "abc";
AIMRT_HL_TRACE(core_.GetLogger(), "test trace log, num: {}, s: {}", 123, s);
AIMRT_HL_DEBUG(core_.GetLogger(), "test debug log, num: {}, s: {}", 123, s);
AIMRT_HL_INFO(core_.GetLogger(), "test info log, num: {}, s: {}", 123, s);
AIMRT_HL_WARN(core_.GetLogger(), "test warn log, num: {}, s: {}", 123, s);
AIMRT_HL_ERROR(core_.GetLogger(), "test error log, num: {}, s: {}", 123, s);
AIMRT_HL_FATAL(core_.GetLogger(), "test fatal log, num: {}, s: {}", 123, s);
```

&emsp;&emsp;如果在当前上下文中定义了默认的日志句柄`GetLogger()`，则可以省略第一个参数，使用以下几个日志宏来打日志：
```cpp
// 定义有日志句柄方法
LoggerRef GetLogger();

// ...

std::string s = "abc";
AIMRT_TRACE("test trace log, num: {}, s: {}", 123, s);
AIMRT_DEBUG("test debug log, num: {}, s: {}", 123, s);
AIMRT_INFO("test info log, num: {}, s: {}", 123, s);
AIMRT_WARN("test warn log, num: {}, s: {}", 123, s);
AIMRT_ERROR("test error log, num: {}, s: {}", 123, s);
AIMRT_FATAL("test fatal log, num: {}, s: {}", 123, s);
```

&emsp;&emsp;开发者可以在模块内定义`GetLogger()`成员方法从而实现模块类内日志功能，也可以在特定命名空间内定义全局`GetLogger()`方法实现特定范围内的全局日志功能。

&emsp;&emsp;日志配置参考如下：
```yaml
aimrt:
  log: # log配置
    core_lvl: TRACE # 内核日志等级，可选项：Trace/Debug/Info/Warn/Error/Fatal/Off，不区分大小写
    default_module_lvl: TRACE # 模块默认日志等级
    writers: # 日志writers。此处只提供两种默认的，可以在代码中通过addwriter接口手动添加
      - type: console_writer # 控制台writer
        color: true # 是否彩色打印
      - type: rotate_file_writer # 文件writer
        path: ./log # 日志文件路径
        filename: normal_example.log # 日志文件名称
        max_file_size_m: 16 # 日志文件最大尺寸，单位m
        max_file_num: 10 # 最大日志文件数量，0代表无限
  module: # 模块配置
    modules: # 模块
      - name: HelloWorldModule # 模块Name接口返回的名称
        log_lvl: TRACE # 模块日志级别

```

&emsp;&emsp;在`aimrt.log`中可以配置框架的日志级别，以及默认的模块日志级别，同时提供几种日志后端供选择。在模块配置`aimrt.module`中，也可以为不同的模块配置不同的日志级别。可配置的日志级别参见以下列表：
- Trace
- Debug
- Info
- Warn
- Error
- Fatal
- Off


## 使用执行器


&emsp;&emsp;在框架启动时，可以配置一个或多个执行器。目前框架只支持线程型执行器，后续可能支持更多的执行器种类。例如框架使用以下配置时：
```yaml
aimrt:
  executor: # 执行器配置
    executors: # 当前先支持thread型/strand型，未来可根据加载的网络模块提供更多类型
      - name: work_thread_pool # 线程池
        type: asio_thread # 类型为asio实现的线程池
        thread_num: 4 # 线程数，不指定则默认单线程
      - name: my_single_thread # 单线程
        type: asio_thread # 类型为asio实现的线程池
```

&emsp;&emsp;当框架启动时，将创建两个执行器：
- 名称为`work_thread_pool`的、包含4个线程的线程池执行器
- 名称为`my_single_thread`的、只有一个线程的单线程执行器

&emsp;&emsp;模块可以通过在初始化时传入的`aimrt::CoreRef`变量的`GetExecutorManager()`方法，获取框架提供的执行器管理器句柄，类型为`ExecutorManagerRef`。可以使用它的`GetExecutor`方法获取具体的执行器句柄：
```cpp
bool HelloWorldModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  // 获取执行器句柄，参数为配置时的执行器名称
  ExecutorRef executor = core_.GetExecutorManager().GetExecutor("work_thread_pool");
  return true;
}
```

&emsp;&emsp;通过获取到的`ExecutorRef`指针，模块可以使用`Execute(task)`方法将任务投递到各个执行器中去执行：
```cpp
bool HelloWorldModule::Start() noexcept {
  // 获取执行器句柄，参数为配置时的执行器名称
  ExecutorRef executor = core_.GetExecutorManager().GetExecutor("work_thread_pool");

  // 将在work_thread_pool线程池中执行投递的任务
  executor_.Execute([logger = core_.GetLogger()]() {
    AIMRT_HL_TRACE(logger, "test execute");
  });

  return true;
}
```

&emsp;&emsp;请注意：仅能在`Start`方法以及之后使用执行器。


## 使用Channel通信

&emsp;&emsp;Channel功能使模块之间可以订阅发布消息。参考[example_normal_channel](https://code.agibot.com/agibot-tech/aimrt/-/tree/main/src/examples/example_normal_channel)示例，我们使用`NormalPublisherModule`模块不断发布一个事件，让`NormalSubscriberModule`模块订阅这个事件。其中订阅和发布模块封装在两个Pkg中，如下所示：
- normal_channel_alpha_pkg
  - NormalPublisherModule
- normal_channel_beta_pkg
  - NormalSubscriberModule

&emsp;&emsp;此时核心目录结构如下：
```
+ src
  + install // 安装时需要的一些文件
    + bin // 启停脚本、配置文件等
      - cfg.yaml
  + module // 模块
    + normal_publisher_module // Publisher模块
      - CMakeLists.txt
      - normal_publisher_module.cc
      - normal_publisher_module.h
    + normal_subscriber_module // Subscriber模块
      - CMakeLists.txt
      - normal_subscriber_module.cc
      - normal_subscriber_module.h
  + pkg // 模块包
    + normal_channel_alpha_pkg // 模块包，编译后是一个动态库
      - CMakeLists.txt
      - pkg_main.cc
    + normal_channel_beta_pkg // 模块包，编译后是一个动态库
      - CMakeLists.txt
      - pkg_main.cc
  + protocols // 协议
    + example // event协议
      - CMakeLists.txt
      - event.proto
```

&emsp;&emsp;其中`src/protocols/example/event.proto`协议文件来定义事件消息的结构，我们可以使用Protobuf来定义。其代码参考如下：
```protobuf
syntax = "proto3";

package aimrt.protocols.example;

message ExampleEventMsg {
  string msg = 1;
  int32 num = 2;
}
```

&emsp;&emsp;然后在`src/protocols/example/CMakeLists.txt`脚本中为其生成桩代码CMake Target：
```cpp
add_protobuf_gencode_target_for_proto_path(
  TARGET_NAME example_pb_gencode
  PROTO_PATH ${CMAKE_CURRENT_SOURCE_DIR}
  GENCODE_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_library(my_namespace::example_pb_gencode ALIAS example_pb_gencode)
```

&emsp;&emsp;这样之后，无论是Publisher端还是Subscriber端，只要链接`my_namespace::example_pb_gencode`这个Target即可使用该协议。例如在作为Publisher端的`NormalPublisherModule`模块下，在其CMakeLists.txt中使用如下代码即可：
```cmake
target_link_libraries(normal_publisher_module PUBLIC my_namespace::example_pb_gencode)
```

&emsp;&emsp;引用之后，在作为Publish端的`NormalPublisherModule`模块中，需要先进行结构的注册，然后才能调用发布方法，示例代码如下：
```cpp
bool NormalPublisherModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  // 注册事件消息结构
  std::string topic_name = "test_topic";
  publisher_ = core_.GetChannel().GetPublisher(topic_name);
  aimrt::channel::RegisterPublishType<aimrt::protocols::example::ExampleEventMsg>(publisher_);

  // 获取要跑任务的执行器句柄
  executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");

  return true;
}

bool NormalPublisherModule::Start() noexcept {
  // 启动一个跑循环的协程
  scope_.spawn(MainLoop());
  return true;
}

void NormalPublisherModule::Shutdown() noexcept {
  // 模块Shutdown时需要结束协程
  run_flag_ = false;
  co::SyncWait(scope_.complete());
}

// 主循环
aimrt::co::Task<void> NormalPublisherModule::MainLoop() {
  try {
    aimrt::co::AimRTScheduler work_thread_pool_scheduler(executor_);

    while (run_flag_) {
      // 等待一段时间
      co_await aimrt::co::ScheduleAfter(
          work_thread_pool_scheduler, std::chrono::microseconds(1000));

      // 创建要发布的数据结构并填数据
      aimrt::protocols::example::ExampleEventMsg msg;
      msg.set_msg("hello!");

      // 发布接口
      aimrt::channel::Publish(publisher_, msg);
    }

  } catch (const std::exception& e) {
    // ...
  }

  co_return;
}
```

&emsp;&emsp;该示例代码中使用了协程作为逻辑流程的组织方式。实际业务使用时可以在任意地方调用Publish接口，不一定非要用协程。

&emsp;&emsp;在订阅端，事件处理的回调方法提供两种接口：
- 智能指针形式接口：
  ```cpp
  // 订阅回调函数
  void EventHandle(const std::shared_ptr<const aimrt::protocols::example::ExampleEventMsg>& data_ptr);

  // 订阅接口
  aimrt::channel::Subscribe<aimrt::protocols::example::ExampleEventMsg>(
        subscriber_, EventHandle);
  ```
- 协程形式接口：
  ```cpp
  // 订阅回调函数
  aimrt::co::Task<void> EventHandle(const aimrt::protocols::example::ExampleEventMsg& data);

  // 订阅接口
  aimrt::channel::SubscribeCo<aimrt::protocols::example::ExampleEventMsg>(
        subscriber_, EventHandle);
  ```

&emsp;&emsp;在作为Subscriber端的`NormalSubscriberModule`模块中，示例代码如下：

```cpp
bool NormalSubscriberModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  // 订阅事件
  std::string topic_name = "test_topic";
  subscriber_ = core_.GetChannel().GetSubscriber(topic_name);
  aimrt::channel::SubscribeCo<aimrt::protocols::example::ExampleEventMsg>(
        subscriber_, std::bind(&NormalSubscriberModule::EventHandle, this, std::placeholders::_1));

  return true;
}

// 事件处理方法
aimrt::co::Task<void> NormalSubscriberModule::EventHandle(
    const aimrt::protocols::example::ExampleEventMsg& data) {
  AIMRT_INFO("Get new pb event, data: {}", aimrt::Pb2CompactJson(data));

  co_return;
}
```

&emsp;&emsp;需要注意的是，无论Publish端的`RegisterPublishType`方法和Subscribe端的`Subscribe`/`SubscribeCo`方法，都必须在模块的`Initialize`方法内调用，不能在初始化之后再注册，否则会返回注册失败。


## 使用rpc通信

&emsp;&emsp;Rpc功能使模块之间可以调用/提供服务。参考[example_normal_rpc](https://code.agibot.com/agibot-tech/aimrt/-/tree/main/src/examples/example_normal_rpc)示例，我们使用`NormalRpcServerModule`模块作为服务端提供一个Rpc服务，让`NormalRpcClientModule`模块创建一个客户端调用这个服务。其中服务端模块和客户端模块封装在两个Pkg中，如下所示：
- normal_rpc_alpha_pkg
  - NormalRpcClientModule
- normal_rpc_beta_pkg
  - NormalRpcServerModule

&emsp;&emsp;此时核心目录结构如下：
```
+ src // 代码
  + install // 安装时需要的一些文件
    + bin // 启停脚本、配置文件等
      - cfg.yaml
  + module // 模块
    + normal_rpc_client_module // client模块
      - CMakeLists.txt
      - normal_rpc_client_module.cc
      - normal_rpc_client_module.h
    + normal_rpc_server_module // server模块
      - CMakeLists.txt
      - normal_rpc_server_module.cc
      - normal_rpc_server_module.h
      - global.cc
      - global.h
      - rpc_service.cc
      - rpc_service.h
  + pkg // 模块包
    + normal_rpc_alpha_pkg // 模块包，编译后是一个动态库
      - CMakeLists.txt
      - pkg_main.cc
    + normal_rpc_beta_pkg // 模块包，编译后是一个动态库
      - CMakeLists.txt
      - pkg_main.cc
  + protocols // 协议
    + example // rpc协议
      - CMakeLists.txt
      - rpc.proto
```

&emsp;&emsp;我们首先需要使用`src/protocols/example/rpc.proto`文件定义RPC协议，参照以下代码：
```protobuf
syntax = "proto3";

import "common.proto";

package aimrt.protocols.example;

message GetFooDataReq {
  string msg = 1;
}

message GetFooDataRsp {
  uint64 code = 1;
  string msg = 2;
  aimrt.protocols.example.ExampleFoo data = 3;
}

service ExampleService {
  rpc GetFooData(GetFooDataReq) returns (GetFooDataRsp);
}
```

&emsp;&emsp;然后在`src/proto/rpc_proto/CMakeLists.txt`脚本中为其生成桩代码target：
```cpp
add_protobuf_gencode_target_for_proto_path(
  TARGET_NAME example_pb_gencode
  PROTO_PATH ${CMAKE_CURRENT_SOURCE_DIR}
  GENCODE_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_library(my_namespace::example_pb_gencode ALIAS example_pb_gencode)

add_protobuf_aimrt_rpc_gencode_target_for_proto_files(
  TARGET_NAME example_aimrt_rpc_gencode
  PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto
  GENCODE_PATH ${CMAKE_CURRENT_BINARY_DIR}
  DEP_PROTO_TARGETS my_namespace::example_pb_gencode)
add_library(my_namespace::example_aimrt_rpc_gencode ALIAS example_aimrt_rpc_gencode)
```

&emsp;&emsp;这样之后，无论是Server端还是Client端，只要链接`my_namespace::example_aimrt_rpc_gencode`这个Target即可使用该协议与RPC方法。例如在作为Server端的`NormalRpcServerModule`模块下，在其CMakeLists.txt中使用如下代码即可：
```cmake
target_link_libraries(normal_rpc_server_module PUBLIC my_namespace::example_aimrt_rpc_gencode)
```


&emsp;&emsp;注意，无论在Servert端还是Client端，RPC的实现或调用目前都是以协程形式的接口提供的，使用者需要实现协程形式的RPC服务方法，并在协程中调用RPC。例如，在作为Server端的`NormalRpcServerModule`模块代码中，要实现具体的RPC逻辑，并注册RPC服务，示例代码如下：

- src/module/normal_rpc_server_module/rpc_service.h
  ```cpp
  // 继承桩代码中的service基类，实现相关接口
  class HardwareServiceImpl : public aimrt::protocols::example::ExampleService {
  public:
    HardwareServiceImpl() = default;
    ~HardwareServiceImpl() override = default;

    aimrt::co::Task<aimrt::rpc::Status> GetFooData(
        aimrt::rpc::ContextRef ctx,
        const ::aimrt::protocols::example::GetFooDataReq& req,
        ::aimrt::protocols::example::GetFooDataRsp& rsp) override;
  };
  ```
- src/module/normal_rpc_server_module/rpc_service.cc
  ```cpp
  // RPC处理函数，协程接口
  aimrt::co::Task<aimrt::rpc::Status> HardwareServiceImpl::GetFooData(
      aimrt::rpc::ContextRef ctx,
      const ::aimrt::protocols::example::GetFooDataReq& req,
      ::aimrt::protocols::example::GetFooDataRsp& rsp) {
    rsp.set_msg("echo " + req.msg());

    AIMRT_INFO("Server handle new rpc call. req: {}, return rsp: {}",
              aimrt::Pb2CompactJson(req), aimrt::Pb2CompactJson(rsp));

    co_return aimrt::rpc::Status();
  }
  ```
- src/module/normal_rpc_server_module/normal_rpc_server_module.cc
  ```cpp
  bool NormalRpcServerModule::Initialize(aimrt::CoreRef core) noexcept {
    core_ = core;

    // 注册RPC服务
    service_ptr_ = std::make_shared<HardwareServiceImpl>();
    core_.GetRpcHandle().RegisterService(service_ptr_);

    return true;
  }
  ```

&emsp;&emsp;在作为Client端的`NormalRpcClientModule`模块代码中，其示例调用代码如下：
```cpp
bool NormalRpcClientModule::Initialize(aimrt::CoreRef core) noexcept {
  core_ = core;

  // 注册RPC客户端方法
  aaimrt::rpc::RegisterClientFunc<aimrt::protocols::example::ExampleServiceProxy>(core_.GetRpcHandle());

  // 获取要跑任务的执行器句柄
  executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");

  return true;
}

bool NormalRpcClientModule::Start() noexcept {
  // 启动一个跑循环的协程
  scope_.spawn(MainLoop());
  return true;
}

void NormalRpcClientModule::Shutdown() noexcept {
  run_flag_ = false;
  co::SyncWait(scope_.complete());
}

// 跑循环的协程
aimrt::co::Task<void> NormalRpcClientModule::MainLoop() {
  try {
    auto proxy = std::make_shared<aimrt::protocols::example::ExampleServiceProxy>(core_.GetRpcHandle());

    aimrt::co::AimRTScheduler work_thread_pool_scheduler(executor_);

    while (run_flag_) {
      co_await aimrt::co::ScheduleAfter(
          work_thread_pool_scheduler, std::chrono::milliseconds(1000));

      // call rpc
      aimrt::protocols::example::GetFooDataReq req;
      aimrt::protocols::example::GetFooDataRsp rsp;
      req.set_msg("hello world");

      auto status = co_await proxy_->GetFooData(req, rsp);
    }

  } catch (const std::exception& e) {
    // ...
  }

  co_return;
}
```

&emsp;&emsp;需要注意的是，无论Server端还是Client端，都有个注册的步骤，要调用一个注册函数，Server端是`RegisterService`，客户端是`RegisterClientFunc`。这两个注册方法都必须在模块的`Initialize`方法内调用，不能在初始化之后再注册，否则会返回注册失败。
