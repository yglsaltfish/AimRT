
# 配置


## 简介

AimRT进程在启动时需要一个配置文件，来定义各个组件的运行时表现。

## 一些通用性说明

### Yaml格式

AimRT采用Yaml作为配置文件格式。YAML 是一种人类可读的数据序列化语言，通常用于编写配置文件。关于Yaml的语法细节，请参考互联网上的文档或[Yaml官方网站](https://yaml.org/)。


### 不区分业务开发语言

AimRT的配置文件不区分业务代码的开发语言，无论是Python还是Cpp都使用同一套标准。


### AimRT框架配置的基本结构

AimRT的配置文件中，所有的框架相关配置都在`aimrt`根节点下。在`aimrt`根节点下包含各个组件的配置节点，基本风格是小写字母+下划线，目前共有**10**个主要组件：


| 节点            | 类型    | 是否可选| 作用 |
| ----            | ----    | ----  | ---- |
| configurator    | map     | 可选  | 配置工具的配置 |
| plugin          | map     | 可选  | 插件配置 |
| main_thread     | map     | 可选  | 主线程配置 |
| allocator       | map     | 可选  | 内存分配器配置 |
| executor        | map     | 可选  | 执行器配置 |
| log             | map     | 可选  | 日志配置 |
| rpc             | map     | 可选  | RPC配置 |
| channel         | map     | 可选  | Channel配置 |
| parameter       | map     | 可选  | 参数配置 |
| module          | map     | 可选  | 模块配置 |

以下是一个简单的示例，先给读者一个感性的印象。关于各个组件的详细配置方法，请参考后续章节：
```yaml
aimrt: # AimRT框架的配置
  configurator: # 【可选】配置工具的配置
    temp_cfg_path: ./cfg/tmp
  plugin: # 【可选】插件配置
    plugins:
      - name: xxx_plugin
        path: ./libmqtt_plugin.so
        options:
          xxx_key: xxx_val
          yyy_key: yyy_val
  main_thread: # 【可选】主线程配置
    name: main_thread
  allocator: # 【可选】内存分配器配置
    # ...
  executor: # 【可选】执行器配置
    executors:
      - name: work_executor
        type: asio_thread
        options:
          thread_num: 2
  log: # 【可选】日志配置
    core_lvl: INFO
    default_module_lvl: INFO
    backends:
      - type: console
  rpc: # 【可选】RPC配置
    backends:
      - type: local
      - type: mqtt
    clients_options:
      - func_name: "(.*)"
        enable_backends: [local]
    servers_options:
      - func_name: "(.*)"
        enable_backends: [local]
  channel: # 【可选】Channel配置
    backends:
      - type: local
      - type: mqtt
    pub_topics_options:
      - topic_name: "(.*)"
        enable_backends: [local]
    sub_topics_options:
      - topic_name: "(.*)"
        enable_backends: [local]
  parameter: # 【可选】参数配置
    # ...
  module: # 【可选】模块配置
    pkgs:
      - path: /path/to/libxxx_pkg.so
        disable_module: [XXXModule]
    modules:
      - name: FooModule
        enable: True
        log_lvl: INFO
        cfg_file_path: /path/to/foo_module_cfg.yaml
      - name: BarModule
        enable: True
        log_lvl: WARN
```


### 业务配置

除了框架的配置，AimRT还支持用户将业务模块的配置也以Yaml的形式写在同一个配置文件中，以模块名称为节点名，示例如下：
```yaml
aimrt: # AimRT框架的配置
  # ...

# 模块自定义配置，以模块名称为节点名
FooModule:
  key_1: val_1
  key_2: val_2

BarModule:
  array:
    - val1
    - val2
  map:
    key_1: val_1
    key_2: val_2

```

当然，如果用户不想要把业务模块配置与AimRT框架配置写在一个文件中，甚至不想要以Yaml格式来写配置，AimRT也可以支持。具体的使用方式请参考`aimrt.configurator：配置`一节。


### 环境变量替换功能

AimRT的配置文件支持替换环境变量。在解析配置文件前，AimRT会将配置文件中形如`${XXX_ENV}`的字符串替换为环境变量`XXX_ENV`的值。注意，如果没有此环境变量，则会替换为空字符串。


### 配置文件Dump功能

如果使用者不确定自己的配置是否正确，可以使用AimRT运行时的`配置Dump功能`，将AimRT解析后的完整配置文件Dump下来，看看和开发者自己的预期是否相符。具体可以参考[CPP运行时接口](cpp_runtime.md)中关于启动参数的章节。



## `aimrt.module`：模块

`aimrt.module`配置项主要用于配置模块的加载信息，以及模块对各个其他组件的特殊配置。这是一个可选配置项，其中的细节配置说明如下：


| 节点                                  | 类型          | 是否可选| 默认值 | 作用 |
| ----                                  | ----          | ----  | ----  | ---- |
| pkgs                     | array         | 可选  | []    | 要加载的Pkg动态库配置 |
| pkgs[i].path             | string        | 必选  | ""    | 要加载的Pkg动态库路径 |
| pkgs[i].disable_module   | string array  | 可选  | []    | 此动态库中要屏蔽的模块名称 |
| modules                  | array         | 可选  | []    | 模块详细配置 |
| modules[i].name          | string        | 必选  | ""    | 模块名称 |
| modules[i].enable        | bool          | 可选  | True  | 是否启用 |
| modules[i].log_lvl       | string        | 可选  | ${aimrt.log.default_module_lvl}    | 模块日志级别 |
| modules[i].cfg_file_path | string        | 可选  | ""    | 自定义模块配置文件路径 |


以下是一个简单的示例：
```yaml
aimrt:
  module: # 【可选】模块配置根节点
    pkgs: # 【可选】要加载的Pkg动态库配置
      - path: /path/to/libxxx_pkg.so # 【必选】so/dll地址
        disable_module: [XXXModule] # 【可选】此动态库中要屏蔽的模块名称。默认全部加载
    modules: # 【可选】模块
      - name: FooModule # 【必选】模块名称
        enable: True # 【可选】是否启用。默认True
        log_lvl: INFO # 【可选】模块日志级别。默认采用Log组件统一的配置
        cfg_file_path: /path/to/foo_module_cfg.yaml # 【可选】自定义模块配置文件路径
      - name: BarModule # 【必选】模块名称
        enable: True # 【可选】是否启用。默认True
        log_lvl: WARN # 【可选】模块日志级别。默认采用Log组件统一的配置

# BarModule的配置
BarModule:
  foo_key: foo_val
  bar_key: bar_val
```

使用时请注意，在`aimrt.module`节点下：
- `pkg`是一个数组，用于要加载的Pkg动态库。
  - `pkgs[i].path`用于配置要加载的Pkg动态库路径。不允许出现重复的Pkg路径。如果Pkg文件不存在，AimRT进程会抛出异常。
  - `pkgs[i].disable_module`用于屏蔽Pkg动态库中指定名称的模块。
- `modules`是一个数组，用于配置各个模块。
  - `modules[i].name`表示模块名称。不允许出现重复的模块名称。
  - `modules[i].log_lvl`用以配置模块日志等级。
    - 如果未配置此项，则默认值是`aimrt.log.default_module_lvl`节点所配置的值。
    - 关于可以配置的日志等级，请参考`aimrt.log`日志章节。
  - `modules[i].cfg_file_path`用以配置自定义模块配置文件路径，此处配置关系到Module接口中`configurator`组件`config_file_path`方法返回的结果，其规则如下：
    - 如果使用者配置了此项，则`configurator`组件的`config_file_path`方法将返回此处配置的字符串内容；
    - 如果使用者未配置此项，且AimRT框架配置文件中也不存在以该模块名称命名的根节点，则`configurator`组件的`config_file_path`方法将返回空字符串。
    - 如果使用者未配置此项，但AimRT框架配置文件中存在以该模块名称命名的根节点，则`configurator`组件的`config_file_path`方法将返回一个临时配置文件路径，此临时配置文件将包含AimRT框架配置文件该模块名称节点下的内容。


## `aimrt.configurator`：配置

`aimrt.configurator`配置项比较简单，用于确定模块配置功能的一些细节。配置项说明如下：


| 节点            | 类型          | 是否可选| 默认值 | 作用 |
| ----            | ----          | ----  | ----  | ---- |
| temp_cfg_path   | string        | 可选  | "./cfg/tmp" | 生成的临时模块配置文件存放路径 |


以下是一个简单的示例：
```yaml
aimrt:
  configurator: # 【可选】配置功能根节点
    temp_cfg_path: ./cfg/tmp # 【可选】生成的临时模块配置文件存放路径
```

`aimrt.configurator`节点的使用说明如下：
- `temp_cfg_path`用于配置生成的临时模块配置文件存放路径。
  - 需要配置一个目录形式的路径。如果配置的目录不存在，则AimRT框架会创建一个。如果创建失败，会抛异常。
  - 如果使用者未为某个模块配置该项，但AimRT框架配置文件中存在以该模块名称命名的根节点，框架会在`temp_cfg_path`所配置的路径下为该模块生成一个临时配置文件，将AimRT框架配置文件中该模块的配置拷贝到此临时配置文件中。


## `aimrt.plugin`：插件

`aimrt.plugin`配置项用于配置插件。配置项说明如下：

| 节点                    | 类型          | 是否可选| 默认值 | 作用 |
| ----                    | ----          | ----  | ----  | ---- |
| plugins                 | array         | 可选  | []    | 各个插件的配置 |
| plugins[i].name         | string        | 必选  | ""    | 插件名称 |
| plugins[i].path         | string        | 可选  | ""    | 插件路径。如果是硬编码注册的插件不需要填 |
| plugins[i].options      | map           | 可选  | -     | 传递给插件的初始化配置，具体内容在各个插件章节介绍 |


以下是一个简单的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: xxx_plugin # 【必选】插件名称
        path: ./libxxx_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          xxx_key: xxx_val
          yyy_key: yyy_val
```

`aimrt.plugin`使用注意点如下：
- `plugins`是一个数组，用于配置各个插件。
  - `plugins[i].name`用于配置插件名称。不允许出现重复的插件名称。
  - 如果配置了`plugins[i].path`，AimRT框架会从该路径下加载对应的插件动态库文件。如果使用者基于App模式硬编码注册插件，则不需要配置此项。
  - `plugins[i].options`是AimRT传递给插件的初始化参数，这部分配置格式由各个插件定义，请参考对应插件的文档。



## `aimrt.main_thread`：主线程执行器


`aimrt.main_thread`配置项用于配置主线程。配置项说明如下：

| 节点                | 类型          | 是否可选| 默认值 | 作用 |
| ----                | ----          | ----  | ----  | ---- |
| name                | string        | 可选  | "aimrt_main"    | 主线程名称 |
| thread_sched_policy | string        | 可选  | ""    | 线程调度策略 |
| thread_bind_cpu     | unsigned int array | 可选 | [] | 绑核配置 |

以下是一个简单的示例：
```yaml
aimrt:
  main_thread: # 【可选】主线程配置根节点
    name: main_thread # 【可选】主线程名称
    thread_sched_policy: SCHED_FIFO:80 # 【可选】线程调度策略
    thread_bind_cpu: [0, 1] # 【可选】绑核配置
```

`aimrt.main_thread`使用注意点如下：
- `name`配置了主线程名称，在实现时调用了操作系统的一些API。如果操作系统不支持，则此项配置无效。
- `thread_sched_policy`配置了线程调度策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setschedparam`这个API来配置。支持的方式包括：`SCHED_OTHER`、`SCHED_FIFO:xx`、`SCHED_RR:xx`。`xx`为该模式下的权重值。详细的解释请参考[pthread_setschedparam官方文档](https://man7.org/linux/man-pages/man3/pthread_setschedparam.3.html)。
- `thread_bind_cpu`配置了绑核策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setaffinity_np`这个API来配置，直接在数组中配置CPU ID即可。参考[pthread_setaffinity_np官方文档](https://man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html)。

## `aimrt.executor`：执行器

`aimrt.executor`配置项用于配置执行器。配置项说明如下：

| 节点                    | 类型      | 是否可选| 默认值 | 作用 |
| ----                    | ----      | ----  | ----  | ---- |
| executors               | array     | 可选  | []    | 执行器列表 |
| executors[i].name       | string    | 必选  | ""    | 执行器名称 |
| executors[i].type       | string    | 必选  | ""    | 执行器类型 |
| executors[i].options    | map       | 可选  | -     | 具体执行器的配置 |

以下是一个简单的示例：
```yaml
aimrt:
  executor: # 【可选】执行器配置根节点
    executors: # 【可选】执行器列表
      - name: xxx_executor # 【必选】执行器名称
        type: asio_thread # 【必选】执行器类型
        options: # 【可选】具体执行器的配置
          thread_num: 2
      - name: yyy_executor # 【必选】执行器名称
        type: tbb_thread # 【必选】执行器类型
        options: # 【可选】具体执行器的配置
          thread_num: 1
```

`aimrt.executor`的配置说明如下：
- `executors`是一个数组，用于配置各个执行器。
  - `executors[i].name`表示执行器名称。不允许出现重复的执行器名称。
  - `executors[i].type`表示执行器类型。AimRT官方提供了几种执行器类型，部分插件也提供了一些执行器类型。
  - `executors[i].options`是AimRT传递给各个执行器的初始化参数，这部分配置格式由各个执行器类型定义，请参考对应执行器类型的文档。

### `asio_thread`执行器

`asio_thread`执行器是一种基于[Asio库](https://github.com/chriskohlhoff/asio)实现的执行器，是一种线程池，可以手动设置线程数，此外它还支持定时调度。其所有的配置项如下：

| 节点                          | 类型                  | 是否可选| 默认值 | 作用 |
| ----                          | ----                  | ----  | ----  | ---- |
| thread_num                    | unsigned int          | 可选  | 1     | 线程数 |
| threads_ched_policy           | string                | 可选  | ""    | 线程调度策略 |
| thread_bind_cpu               | unsigned int array    | 可选  | []    | 绑核配置 |
| timeout_alarm_threshold_us    | unsigned int          | 可选  | 1000000 | 调度超时告警阈值，单位：微秒 |


以下是一个简单的示例：
```yaml
aimrt:
  executor: # 【可选】执行器配置根节点
    executors: # 【可选】执行器列表
      - name: test_asio_thread_executor # 【必选】执行器名称
        type: asio_thread # 【必选】执行器类型
        options: # 【可选】具体执行器的配置
          thread_num: 2 # 【可选】线程数
          thread_sched_policy: SCHED_FIFO:80 # 【可选】线程调度策略
          thread_bind_cpu: [0, 1] # 【可选】绑核配置
          timeout_alarm_threshold_us: 1000 # 【可选】调度超时告警阈值，单位：微秒
```

使用注意点如下：
- `thread_num`配置了线程数，默认为1。当线程数配置为 1 时为线程安全执行器，否则是线程不安全的。
- `thread_sched_policy`配置了线程调度策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setschedparam`这个API来配置。支持的方式包括：`SCHED_OTHER`、`SCHED_FIFO:xx`、`SCHED_RR:xx`。`xx`为该模式下的权重值。详细的解释请参考[pthread_setschedparam官方文档](https://man7.org/linux/man-pages/man3/pthread_setschedparam.3.html)。
- `thread_bind_cpu`配置了绑核策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setaffinity_np`这个API来配置，直接在数组中配置CPU ID即可。参考[pthread_setaffinity_np官方文档](https://man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html)。
- `timeout_alarm_threshold_us`配置了一个调度超时告警的阈值。当进行定时调度时，如果CPU负载太重、或队列中任务太多，导致超过设定的时间才调度到，则会打印一个告警日志。



### `asio_strand`执行器

`asio_strand`执行器是一种依附于`asio_thread`执行器的伪执行器，基于Asio库的strand实现。它不能独立存在，并不拥有实际的线程，它在运行过程中会将任务交给绑定的`asio_thread`执行器来实际执行。但是它保证线程安全，也支持定时调度。其所有的配置项如下：

| 节点                              | 类型          | 是否可选| 默认值 | 作用 |
| ----                              | ----          | ----  | ----  | ---- |
| bind_asio_thread_executor_name    | string        | 必选  | ""     | 绑定的asio_thread执行器名称 |
| timeout_alarm_threshold_us        | unsigned int  | 可选  | 1000000 | 调度超时告警阈值，单位：微秒 |


以下是一个简单的示例：
```yaml
aimrt:
  executor: # 【可选】执行器配置根节点
    executors: # 【可选】执行器列表
      - name: test_asio_thread_executor # 【必选】执行器名称
        type: asio_thread # 【必选】执行器类型
        options: # 【可选】具体执行器的配置
          thread_num: 2 # 【可选】线程数
      - name: test_asio_strand_executor # 【必选】执行器名称
        type: asio_strand # 【必选】执行器类型
        options: # 【可选】具体执行器的配置
          bind_asio_thread_executor_name: test_asio_thread_executor # 【必选】绑定的asio_thread执行器名称
          timeout_alarm_threshold_us: 1000 # 【可选】调度超时告警阈值，单位：微秒
```

使用注意点如下：
- 通过`bind_asio_thread_executor_name`配置项来绑定`asio_thread`类型的执行器。如果指定名称的执行器不存在、或不是`asio_thread`类型，则会在初始化时抛出异常。
- `timeout_alarm_threshold_us`配置了一个调度超时告警的阈值。当进行定时调度时，如果CPU负载太重、或队列中任务太多，导致超过设定的时间才调度到，则会打印一个告警日志。


### `tbb_thread`执行器

`tbb_thread`是一种基于[oneTBB库](https://github.com/oneapi-src/oneTBB)的无锁并发队列实现的高性能无锁线程池，可以手动设置线程数，但它不支持定时调度。其所有的配置项如下：

| 节点                          | 类型                  | 是否可选| 默认值 | 作用 |
| ----                          | ----                  | ----  | ----  | ---- |
| thread_num                    | unsigned int          | 可选  | 1     | 线程数 |
| threads_ched_policy           | string                | 可选  | ""    | 线程调度策略 |
| thread_bind_cpu               | unsigned int array    | 可选  | []    | 绑核配置 |
| timeout_alarm_threshold_us    | unsigned int          | 可选  | 1000000 | 调度超时告警阈值，单位：微秒 |



以下是一个简单的示例：
```yaml
aimrt:
  executor: # 【可选】执行器配置根节点
    executors: # 【可选】执行器列表
      - name: test_tbb_thread_executor # 【必选】执行器名称
        type: tbb_thread # 【必选】执行器类型
        options: # 【可选】具体执行器的配置
          thread_num: 2 # 【可选】线程数
          thread_sched_policy: SCHED_FIFO:80 # 【可选】线程调度策略
          thread_bind_cpu: [0, 1] # 【可选】绑核配置
          timeout_alarm_threshold_us: 1000 # 【可选】调度超时告警阈值，单位：微秒
```


使用注意点如下：
- `thread_num`配置了线程数，默认为1。当线程数配置为 1 时为线程安全执行器，否则是线程不安全的。
- `thread_sched_policy`配置了线程调度策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setschedparam`这个API来配置。支持的方式包括：`SCHED_OTHER`、`SCHED_FIFO:xx`、`SCHED_RR:xx`。`xx`为该模式下的权重值。详细的解释请参考[pthread_setschedparam官方文档](https://man7.org/linux/man-pages/man3/pthread_setschedparam.3.html)。
- `thread_bind_cpu`配置了绑核策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setaffinity_np`这个API来配置，直接在数组中配置CPU ID即可。参考[pthread_setaffinity_np官方文档](https://man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html)。
- `timeout_alarm_threshold_us`配置了一个调度超时告警的阈值。当进行定时调度时，如果CPU负载太重、或队列中任务太多，导致超过设定的时间才调度到，则会打印一个告警日志。



### `time_wheel`执行器

`time_wheel`执行器是一种基于时间轮实现的执行器，一般用于有大量定时任务、且对定时精度要求不高的场景，例如RPC超时处理。它会启动一个单独的线程跑时间轮，同时也支持将具体的任务投递到其他执行器中执行。其所有的配置项如下：


| 节点                  | 类型                  | 是否可选| 默认值 | 作用 |
| ----                  | ----                  | ----  | ----  | ---- |
| bind_executor         | string                | 可选  | ""    | 绑定的执行器 |
| dt_us                 | unsigned int          | 可选  | 1000  | 时间轮tick的间隔，单位：微秒 |
| wheel_size            | unsigned int array    | 可选  | [1000, 600]    | 各个时间轮的大小 |
| threads_ched_policy   | string                | 可选  | ""    | 时间轮线程的调度策略 |
| thread_bind_cpu       | unsigned int array    | 可选  | []    | 时间轮线程的绑核配置 |


以下是一个简单的示例：
```yaml
aimrt:
  executor: # 【可选】执行器配置根节点
    executors: # 【可选】执行器列表
      - name: test_tbb_thread_executor # 【必选】执行器名称
        type: tbb_thread # 【必选】执行器类型
        options: # 【可选】具体执行器的配置
          thread_num: 2 # 【可选】线程数
      - name: test_time_wheel_executor # 【必选】执行器名称
        type: time_wheel # 【必选】执行器类型
        options: # 【可选】具体执行器的配置
          bind_executor: test_tbb_thread_executor # 【可选】绑定的执行器
          dt_us: 1000 # 【可选】时间轮tick的间隔，单位：微秒
          wheel_size: [1000, 600] # 【可选】各个时间轮的大小
          thread_sched_policy: SCHED_FIFO:80 # 【可选】时间轮线程的调度策略
          thread_bind_cpu: [0] # 【可选】时间轮线程的绑核配置
```

使用注意点如下：
- `bind_executor`用于配置绑定的执行器，从而在时间达到后将任务投递到绑定的执行器里具体执行。
  - 如果不绑定其他执行器，则所有的任务都会在时间轮线程里执行，有可能阻塞时间轮的Tick。
  - 如果不绑定其他执行器，则本执行器是线程安全的。如果绑定其他执行器，则线程安全性与绑定的执行器一致。
  - 如果绑定的执行器不存在，则会在初始化时抛出一个异常。
- `dt_us`是时间轮算法的一个参数，表示Tick的间隔。间隔越大，定时调度的精度越低，但越节省CPU资源。
- `wheel_size`是时间轮算法的另一个参数，表示各个时间轮的大小。比如默认的参数`[1000, 600]`表示有两个时间轮，第一个轮的刻度是1000，第二个轮的刻度是600。如果Tick时间是 1ms，则第一个轮的完整时间是 1s，第二个轮的完整时间是 10min。一般来说，要让可能的定时时间都落在轮内最好。
- `thread_sched_policy`配置了线程调度策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setschedparam`这个API来配置。支持的方式包括：`SCHED_OTHER`、`SCHED_FIFO:xx`、`SCHED_RR:xx`。`xx`为该模式下的权重值。详细的解释请参考[pthread_setschedparam官方文档](https://man7.org/linux/man-pages/man3/pthread_setschedparam.3.html)。
- `thread_bind_cpu`配置了绑核策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setaffinity_np`这个API来配置，直接在数组中配置CPU ID即可。参考[pthread_setaffinity_np官方文档](https://man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html)。


## `aimrt.log`：日志

`aimrt.log`配置项用于配置日志。配置项说明如下：

| 节点                | 类型      | 是否可选| 默认值 | 作用 |
| ----                | ----      | ----  | ----  | ---- |
| core_lvl            | string    | 可选  | "Info" | 框架日志等级 |
| default_module_lvl  | string    | 可选  | "Info" | 默认的模块日志等级 |
| backends            | array     | 可选  | ""    | 日志后端列表 |
| backends[i].type    | string    | 必选  | ""    | 日志后端类型 |
| backends[i].options | map       | 可选  | -     | 具体日志后端的配置 |

其中，日志等级可选项包括以下几种（不区分大小写）：
- Trace
- Debug
- Info
- Warn
- Error
- Fatal
- Off

以下是一个简单的示例：
```yaml
aimrt:
  log: # 【可选】log配置
    core_lvl: INFO # 【可选】内核日志等级，可选项：Trace/Debug/Info/Warn/Error/Fatal/Off，不区分大小写
    default_module_lvl: INFO # 【可选】模块默认日志等级
    backends: # 【可选】日志backends
      - type: console # 【必选】日志后端类型
      - type: rotate_file # 【必选】日志后端类型
        options: # 【可选】具体日志后端的配置
          path: ./log
          filename: examples_cpp_executor_real_time.log
```

`aimrt.log`的配置说明如下：
- `core_lvl`表示AimRT运行时内核的日志等级，内核日志一般设为Info级别即可。
- `default_module_lvl`默认的模块日志等级。
- `backends`是一个数组，用于注册各个日志后端。
  - `backends[i].type`是日志后端的类型。AimRT官方提供了几种日志后端，部分插件也提供了一些日志后端类型。部分后端允许重复注册，详情请参考对应后端类型的文档。
  - `backends[i].options`是AimRT传递给各个日志后端的初始化参数，这部分配置格式由各个日志后端类型定义，请参考对应日志后端类型的文档。


### `console`控制台日志后端


`console`日志后端是AimRT官方提供的一种日志后端，用于将日志打印到控制台上。其所有的配置项如下：


| 节点                  | 类型              | 是否可选| 默认值 | 作用 |
| ----                  | ----              | ----  | ----  | ---- |
| color                 | bool              | 可选  | true  | 是否要彩色打印 |
| module_filter         | string            | 可选  | "(.*)"| 模块过滤器 |
| log_executor_name     | string            | 可选  | ""    | 日志执行器。默认使用主线程 |


以下是一个简单的示例：
```yaml
aimrt:
  executor:
    executors:
      - name: test_log_executor # 配置一个线程安全的执行器做日志执行器
        type: tbb_thread
        options:
          thread_num: 1
  log: # 【可选】log配置
    core_lvl: INFO # 【可选】内核日志等级，可选项：Trace/Debug/Info/Warn/Error/Fatal/Off，不区分大小写
    default_module_lvl: INFO # 【可选】模块默认日志等级
    backends: # 【可选】日志backends
      - type: console # 【必选】日志后端类型
        options: # 【可选】具体日志后端的配置
          color: true # 【可选】是否要彩色打印
          module_filter: "(.*)" # 【可选】模块过滤器
          log_executor_name: test_log_executor.log # 【可选】日志执行器。默认使用主线程
```

使用注意点如下：
- `console`日志后端不允许重复注册，一个AimRT实例中只允许注册一个。
- `color`配置了是否要彩色打印。此项配置有可能在一些操作系统不支持。
- `module_filter`支持以正则表达式的形式，来配置哪些模块的日志可以通过本后端处理。这与模块日志等级不同，模块日志等级是全局的、先决的、影响所有日志后端的，而这里的配置只影响本后端。
- `log_executor_name`配置了日志执行器。要求日志执行器是线程安全的，如果没有配置，则默认使用主线程打印日志。


### `rotate_file`滚动文件日志后端

`rotate_file`日志后端是AimRT官方提供的一种日志后端，用于将日志打印到文件中。其所有的配置项如下：


| 节点                  | 类型              | 是否可选| 默认值 | 作用 |
| ----                  | ----              | ----  | ----  | ---- |
| path                  | string            | 必选  | "./log"  | 日志文件存放目录 |
| filename              | string            | 必选  | "aimrt.log" | 日志文件基础名称 |
| max_file_size_m       | unsigned int      | 可选  | 16    | 日志文件最大尺寸，单位：Mb |
| max_file_num          | unsigned int      | 可选  | 100   | 日志文件最大数量。0表示无上限 |
| module_filter         | string            | 可选  | "(.*)"| 模块过滤器 |
| log_executor_name     | string            | 可选  | ""    | 日志执行器。默认使用主线程 |


以下是一个简单的示例：
```yaml
aimrt:
  executor:
    executors:
      - name: test_log_executor # 配置一个线程安全的执行器做日志执行器
        type: tbb_thread
        options:
          thread_num: 1
  log: # 【可选】log配置
    core_lvl: INFO # 【可选】内核日志等级，可选项：Trace/Debug/Info/Warn/Error/Fatal/Off，不区分大小写
    default_module_lvl: INFO # 【可选】模块默认日志等级
    backends: # 【可选】日志backends
      - type: rotate_file # 【必选】日志后端类型
        options: # 【可选】具体日志后端的配置
          path: ./log # 【必选】日志文件存放目录
          filename: example.log # 【必选】日志文件基础名称
          max_file_size_m: 4 # 【可选】日志文件最大尺寸，单位：Mb
          max_file_num: 10 # 【可选】日志文件最大数量。0表示无上限
          module_filter: "(.*)" # 【可选】模块过滤器
          log_executor_name: test_log_executor.log # 【可选】日志执行器。默认使用主线程
```

使用注意点如下：
- `rotate_file`日志后端允许重复注册，业务可以基于这个特点，将不同模块的日志打印到不同文件中。
- `path`配置了日志文件的存放路径。如果路径不存在，则会创建一个。如果创建失败，会抛异常。
- `filename`配置了日志文件基础名称。
- 当单个日志文件尺寸超过`max_file_size_m`配置的大小后，就会新建一个日志文件，同时将老的日志文件重命名，加上`_x`这样的后缀。
- 当日志文件数量超过`max_file_num`配置的值后，就会将最老的日志文件删除。如果配置为0，则表示永远不会删除。
- `module_filter`支持以正则表达式的形式，来配置哪些模块的日志可以通过本后端处理。这与模块日志等级不同，模块日志等级是全局的、先决的、影响所有日志后端的，而这里的配置只影响本后端。

## `aimrt.channel`：Channel

`aimrt.channel`配置项用于配置Channel功能。详细配置项说明如下：

| 节点                                | 类型      | 是否可选| 默认值 | 作用 |
| ----                                | ----      | ----  | ----  | ---- |
| backends                            | array     | 可选  | []    | Channel后端列表 |
| backends[i].type                    | string    | 必选  | ""    | Channel后端类型 |
| backends[i].options                 | map       | 可选  | -     | 具体Channel后端的配置 |
| pub_topics_options                  | array     | 可选  | ""    | Channel Pub Topic配置 |
| pub_topics_options[i].topic_name    | string    | 必选  | ""    | Channel Pub Topic名称，支持正则表达式 |
| pub_topics_options[i].enable_backends | string array | 必选  | [] | Channel Pub Topic允许使用的Channel后端列表 |
| sub_topics_options                  | array     | 可选  | ""    | Channel Sub Topic配置 |
| sub_topics_options[i].topic_name    | string    | 必选  | ""    | Channel Sub Topic名称，支持正则表达式 |
| sub_topics_options[i].enable_backends | string array | 必选  | [] | Channel Sub Topic允许使用的Channel后端列表 |


以下是一个简单的示例：
```yaml
aimrt:
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: local # 【必选】Channel后端类型
      - type: mqtt # 【必选】Channel后端类型
    pub_topics_options: # 【可选】Channel Pub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Pub Topic名称，支持正则表达式
        enable_backends: [local] # 【必选】Channel Pub Topic允许使用的Channel后端列表
    sub_topics_options: # 【可选】Channel Sub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Sub Topic名称，支持正则表达式
        enable_backends: [local] # 【必选】Channel Sub Topic允许使用的Channel后端列表
```

`aimrt.channel`的详细配置说明如下：
- `backends`是一个数组，用于配置各个Channel后端。
  - `backends[i].type`是Channel后端的类型。AimRT官方提供了`local`后端，部分插件也提供了一些Channel后端类型。
  - `backends[i].options`是AimRT传递给各个Channel后端的初始化参数，这部分配置格式由各个Channel后端类型定义，请参考对应Channel后端类型的文档。
- `pub_topics_options`和`sub_topics_options`是一个规则列表，用于控制各个`Topic`在发布或订阅消息时使用的Channel后端规则，其中：
  - `topic_name`表示本条规则的`Topic`名称，以正则表达式形式配置，如果`Topic`名称命中了该正则表达式，则会应用该条规则。
  - `enable_backends`是一个字符串数组，表示如果`Topic`名称命中了本条规则，则将该`Topic`下发布的所有消息投递到这个列表中的所有Channel后端进行处理。注意：
    - 该数组中出现的名称都必须要在`backends`中配置过。
    - 该数组配置的Channel后端顺序决定了消息投递到各个Channel后端进行处理的顺序。
  - 采用由上往下的顺序检查命中的规则，当某个`Topic`命中某条规则后，则不会针对此`Topic`再检查后面的规则。


在AimRT中，Channel的前端接口和后端实现是解耦的，在接口中`Publish`一个消息后最终是要Channel后端来进行正真的发布动作，消息通常会在调用`Publish`之后，在当前线程里顺序的投递到各个Channel后端中进行处理。大部分Channel后端是异步处理消息的，但有些特殊的后端-例如`local`后端，可以配置成阻塞的调用订阅端回调。因此，`Publish`方法到底会阻塞多久是未定义的，与具体的后端配置相关。


***TODO: CTX这部分的功能还在开发中，文档后续再补充***

此外，Channel的发布和订阅接口还可以包含一个Ctx参数，这个Ctx参数包含一个K-V形式的map，其中有一部分参数用于在运行阶段配置Channel的表现，这部分参数如下：

| Key值                     | Val含义     | 作用 |
| ----                      | ----        | ----  | 
| TODO                      | TODO        |  TODO |

此外还有一部分参数是给具体的Channel后端使用的，此部分详见各个Channel后端的文档。



### `local`类型Channel后端


`local`类型的Channel后端是AimRT官方提供的一种Channel后端，用于将消息发布到同进程中的其他模块，它会自动判断发布端和订阅端是否在同一个`Pkg`内，从而采用各种方式进行性能的优化。其所有的配置项如下：


| 节点                            | 类型    | 是否可选| 默认值 | 作用 |
| ----                            | ----    | ----  | ----  | ---- |
| subscriber_use_inline_executor  | bool    | 可选  | true  | 订阅端回调是否使用inline执行器 |
| subscriber_executor             | string  | subscriber_use_inline_executor为false时必选  | "" | 订阅端回调使用的执行器名称 |


以下是一个简单的示例：
```yaml
aimrt:
  executor:
    executors:
      - name: work_thread_pool
        type: asio_thread
        options:
          thread_num: 4
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: local # 【必选】Channel后端类型
        options: # 【可选】具体Channel后端的配置
          subscriber_use_inline_executor: false # 【可选】订阅端回调是否使用inline执行器
          subscriber_executor: work_thread_pool # 【subscriber_use_inline_executor为false时必选】订阅端回调使用的执行器名称
    pub_topics_options: # 【可选】Channel Pub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Pub Topic名称，支持正则表达式
        enable_backends: [local] # 【必选】Channel Pub Topic允许使用的Channel后端列表
    sub_topics_options: # 【可选】Channel Sub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Sub Topic名称，支持正则表达式
        enable_backends: [local] # 【必选】Channel Sub Topic允许使用的Channel后端列表
```

使用注意点如下：
- `subscriber_use_inline_executor`如果配置为true，则直接使用发布端的执行器来执行订阅端的回调，会阻塞发布端的Publish方法直到所有的订阅端回调都执行完成。
- `subscriber_executor`仅在subscriber_use_inline_executor为false时生效，后端会将订阅端的回调都投递进此执行器中异步执行，发布端的Publish方法会立即返回。

## `aimrt.rpc`：RPC

`aimrt.rpc`配置项用于配置RPC功能。详细配置项说明如下：


| 节点                                | 类型      | 是否可选| 默认值 | 作用 |
| ----                                | ----      | ----  | ----  | ---- |
| backends                            | array     | 可选  | []    | RPC后端列表 |
| backends[i].type                    | string    | 必选  | ""    | RPC后端类型 |
| backends[i].options                 | map       | 可选  | -     | 具体RPC后端的配置 |
| clients_options                     | array     | 可选  | ""    | RPC Client配置 |
| clients_options[i].func_name        | string    | 必选  | ""    | RPC Client名称，支持正则表达式 |
| clients_options[i].enable_backends  | string array | 必选  | [] | RPC Client允许使用的RPC后端列表 |
| servers_options                     | array     | 可选  | ""    | RPC Server配置 |
| servers_options[i].func_name        | string    | 必选  | ""    | RPC Server名称，支持正则表达式 |
| servers_options[i].enable_backends  | string array | 必选  | [] | RPC Server允许使用的RPC后端列表 |


以下是一个简单的示例：
```yaml
aimrt:
  rpc: # 【可选】RPC配置根节点
    backends: # 【可选】RPC后端列表
      - type: local # 【必选】RPC后端类型
      - type: mqtt # 【必选】RPC后端类型
    clients_options: # 【可选】RPC Client配置
      - func_name: "(.*)" # 【必选】RPC Client名称，支持正则表达式
        enable_backends: [local] # 【必选】RPC Client允许使用的RPC后端列表
    servers_options: # 【可选】RPC Server配置
      - func_name: "(.*)" # 【必选】RPC Server名称，支持正则表达式
        enable_backends: [local] # 【必选】RPC Server允许使用的RPC后端列表
```



`aimrt.rpc`的配置说明如下：
- `backends`是一个数组，用于配置各个Rpc后端。
  - `backends[i].type`是Rpc后端的类型。AimRT官方提供了`local`后端，部分插件也提供了一些Rpc后端类型。
  - `backends[i].options`是AimRT传递给各个Rpc后端的初始化参数，这部分配置格式由各个Rpc后端类型定义，请参考对应Rpc后端类型的文档。
- `clients_options`和`servers_options`是一个规则列表，用于控制各个RPC方法在发起调用或处理调用时使用的Rpc后端规则，其中：
  - `func_name`表示本条规则的RPC方法名称，以正则表达式形式配置，如果RPC方法名称命中了该正则表达式，则会应用该条规则。
  - `enable_backends`是一个字符串数组，表示如果RPC方法名称命中了本条规则，则此数组就定义了该RPC方法能被处理的RPC后端。注意，该数组中出现的名称都必须要在`backends`中配置过。
  - 采用由上往下的顺序检查命中的规则，当某个RPC方法命中某条规则后，则不会针对此RPC方法再检查后面的规则。


在AimRT中，RPC的前端接口和后端实现是解耦的，当开发者使用接口发起一个RPC调用，最终是要RPC后端来执行正真的RPC调用操作。

当Client端接口层发起一个RPC请求后，AimRT框架会根据以下规则，在多个RPC后端中选择一个进行实际的处理：
- AimRT框架会先根据`clients_options`配置确定某个RPC方法能被处理的RPC后端列表。
- AimRT框架会先解析传入的CTX里Meta参数中的`AIMRT_RPC_CONTEXT_KEY_TO_ADDR`项，如果其中手动配置了形如`xxx://yyy,zzz`这样的URL，则会解析出`xxx`字符串，并寻找同名的RPC后端进行处理。
- 如果没有配置CTX参数，则根据该RPC方法能被处理的RPC后端列表顺序，依次尝试进行处理，直到遇到第一个真正进行处理的后端。

Server端相对来说规则就比较简单，会根据`servers_options`的配置，接收并处理其中各个RPC后端传递过来的请求。


***TODO: CTX这部分的功能还在开发中，文档后续再补充***

此外，RPC的请求和处理接口还可以包含一个Ctx参数，这个Ctx参数包含一个K-V形式的map，其中有一部分参数用于在运行阶段配置Rpc的表现，这部分参数如下：

| Key值                     | Val含义     | 作用 |
| ----                      | ----        | ----  | 
| TODO                      | TODO        |  TODO |

此外还有一部分参数是给具体的Rpc后端使用的，此部分详见各个Rpc后端的文档。



### `local`类型Rpc后端


`local`类型的Rpc后端是AimRT官方提供的一种Rpc后端，用于请求同进程中的其他模块提供的RPC，它会自动判断Client端和Server端是否在同一个`Pkg`内，从而采用各种方式进行性能的优化。其所有的配置项如下：


| 节点                          | 类型      | 是否可选| 默认值 | 作用 |
| ----                          | ----      | ----  | ----  | ---- |
| timeout_executor              | string    | 可选  | ""    | Client端RPC超时情况下的执行器 |


以下是一个简单的示例：
```yaml
aimrt:
  executor:
    executors:
      - name: timeout_handle
        type: time_wheel
  rpc: # 【可选】RPC配置根节点
    backends: # 【可选】RPC后端列表
      - type: local # 【必选】RPC后端类型
        options: # 【可选】RPC Client配置
          timeout_executor: timeout_handle # 【可选】Client端RPC超时情况下的执行器
    clients_options: # 【可选】RPC Client配置
      - func_name: "(.*)" # 【必选】RPC Client名称，支持正则表达式
        enable_backends: [local] # 【必选】RPC Client允许使用的RPC后端列表
    servers_options: # 【可选】RPC Server配置
      - func_name: "(.*)" # 【必选】RPC Server名称，支持正则表达式
        enable_backends: [local] # 【必选】RPC Server允许使用的RPC后端列表
```

使用注意点如下：
- Server的执行器将使用Client调用时的执行器。同样，Client调用结束后的执行器将使用Server返回时的执行器。
- 如果client和server在一个Pkg中，那么Req、Rsp的传递将直接通过指针来进行；如果client和server在一个AimRT进程中，但在不同的Pkg里，那么Req、Rsp将会进行一次序列化/反序列化再进行传递。
- timeout功能仅在client和server位于不同Pkg时生效。如果client和server在一个Pkg中，那么为了保证Client端Req、Rsp的生命周期能覆盖Server端Req、Rsp的生命周期，timeout功能将不会生效。



## `aimrt.parameter`：参数


***TODO：parameter 暂无可配置的参数***



