
# 配置

[TOC]

## 简介

&emsp;&emsp;AimRT进程在启动时需要一个配置文件，来定义各个组件的运行时表现。

## 一些通用性说明

### Yaml格式

&emsp;&emsp;AimRT采用Yaml作为配置文件格式。YAML 是一种人类可读的数据序列化语言，通常用于编写配置文件。关于Yaml的语法细节，请参考互联网上的文档或[Yaml官方网站](https://yaml.org/)。


### 不区分业务开发语言

&emsp;&emsp;AimRT的配置文件不区分业务代码的开发语言，无论是Python还是Cpp都使用同一套标准。


### AimRT框架配置的基本结构

&emsp;&emsp;AimRT的配置文件中，所有的框架相关配置都在`aimrt`根节点下。在`aimrt`根节点下包含各个组件的配置节点，基本风格是小写字母+下划线，示例如下：
```yaml
aimrt: # AimRT框架的配置
  module: # 模块配置
    # ...
  log: # 日志配置
    # ...
```


### 业务配置

&emsp;&emsp;除了框架的配置，AimRT还支持用户将业务模块的配置也以Yaml的形式写在同一个配置文件中，以模块名称为节点名，示例如下：
```yaml
aimrt: # AimRT框架的配置
  # ...

# 模块自定义配置，以模块名称为节点名
MyModule:
  foo_key: foo_val
  bar_key: bar_val
```

&emsp;&emsp;当然，如果用户不想要把业务模块配置与AimRT框架配置写在一个文件中，甚至不想要以Yaml格式来写配置，AimRT也可以支持。具体的使用方式请参考`aimrt.configurator：配置`一节。


### 环境变量替换功能

&emsp;&emsp;AimRT的配置文件支持替换环境变量。在解析配置文件前，AimRT会将配置文件中形如`${XXX_ENV}`的字符串替换为环境变量`XXX_ENV`的值。注意，如果没有此环境变量，则会替换为空字符串。


### 配置文件Dump功能

&emsp;&emsp;如果使用者不确定自己的配置是否正确，可以使用AimRT运行时的`配置Dump功能`，将AimRT解析后的完整配置文件Dump下来，看看和开发者自己的预期是否相符。具体可以参考[CPP运行时接口](interface/cpp_runtime.md)中关于启动参数的章节。



## `aimrt.module`：模块

&emsp;&emsp;`aimrt.module`配置项主要用于配置模块的加载信息，以及模块对各个其他组件的特殊配置。配置项说明如下：


| 节点                                  | 类型          | 是否可选| 默认值 | 作用 |
| ----                                  | ----          | ----  | ----  | ---- |
| aimrt.module                          | map           | 可选  | -     | 模块配置根节点 |
| aimrt.module.pkgs                     | array         | 可选  | []    | 要加载的Pkg动态库配置 |
| aimrt.module.pkgs[i].path             | string        | 必选  | ""    | 要加载的Pkg动态库路径 |
| aimrt.module.pkgs[i].disable_module   | string array  | 可选  | []    | 此动态库中要屏蔽的模块名称 |
| aimrt.module.modules                  | array         | 可选  | []    | 模块详细配置 |
| aimrt.module.modules[i].name          | string        | 必选  | ""    | 模块名称 |
| aimrt.module.modules[i].enable        | bool          | 可选  | True  | 是否启用 |
| aimrt.module.modules[i].log_lvl       | string        | 可选  | ${aimrt.log.default_module_lvl}    | 模块日志级别 |
| aimrt.module.modules[i].cfg_file_path | string        | 可选  | ""    | 自定义模块配置文件路径 |


&emsp;&emsp;以下是一个简单的示例：
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

&emsp;&emsp;使用时请注意，在`aimrt.module`节点下：
- `pkgs[i].path`用于配置要加载的Pkg动态库路径。不允许出现重复的Pkg路径。如果Pkg文件不存在，AimRT进程会抛出异常。
- `pkgs[i].disable_module`用于屏蔽Pkg动态库中指定名称的模块。
- `modules[i].name`表示模块名称。不允许出现重复的模块名称。
- `modules[i].log_lvl`用以配置模块日志等级。
  - 如果未配置此项，则默认值是`aimrt.log.default_module_lvl`节点所配置的值。
  - 关于可以配置的日志等级，请参考`aimrt.log`日志章节。
- `modules[i].cfg_file_path`用以配置自定义模块配置文件路径，此处配置关系到Module接口中`configurator`组件`config_file_path`方法返回的结果，其规则如下：
  - 如果使用者配置了此项，则`configurator`组件的`config_file_path`方法将返回此处配置的字符串内容；
  - 如果使用者未配置此项，且AimRT框架配置文件中也不存在以该模块名称命名的根节点，则`configurator`组件的`config_file_path`方法将返回空字符串。
  - 如果使用者未配置此项，但AimRT框架配置文件中存在以该模块名称命名的根节点，则`configurator`组件的`config_file_path`方法将返回一个临时配置文件路径，此临时配置文件将包含AimRT框架配置文件该模块名称节点下的内容。


## `aimrt.configurator`：配置

&emsp;&emsp;`aimrt.configurator`配置项比较简单，用于确定模块配置功能的一些细节。配置项说明如下：


| 节点                                  | 类型          | 是否可选| 默认值 | 作用 |
| ----                                  | ----          | ----  | ----  | ---- |
| aimrt.configurator                    | map           | 可选  | -     | 配置功能根节点 |
| aimrt.configurator.temp_cfg_path      | string        | 可选  | "./cfg/tmp" | 生成的临时模块配置文件存放路径 |


&emsp;&emsp;以下是一个简单的示例：
```yaml
aimrt:
  configurator: # 【可选】配置功能根节点
    temp_cfg_path: ./cfg/tmp # 【可选】生成的临时模块配置文件存放路径
```

&emsp;&emsp;`aimrt.configurator`节点的使用说明如下：
- `temp_cfg_path`用于配置生成的临时模块配置文件存放路径。
  - 需要配置一个目录形式的路径。如果配置的目录不存在，则AimRT框架会创建一个。如果创建失败，会抛异常。
  - 如果使用者未为某个模块配置该项，但AimRT框架配置文件中存在以该模块名称命名的根节点，框架会在`temp_cfg_path`所配置的路径下为该模块生成一个临时配置文件，将AimRT框架配置文件中该模块的配置拷贝到此临时配置文件中。


## `aimrt.plugin`：插件

&emsp;&emsp;`aimrt.plugin`配置项用于配置插件。配置项说明如下：

| 节点                                  | 类型          | 是否可选| 默认值 | 作用 |
| ----                                  | ----          | ----  | ----  | ---- |
| aimrt.plugin                          | map           | 可选  | -     | 插件配置根节点 |
| aimrt.plugin.plugins                  | array         | 可选  | []    | 各个插件的配置 |
| aimrt.plugin.plugins[i].name          | string        | 必选  | ""    | 插件名称 |
| aimrt.plugin.plugins[i].path          | string        | 可选  | ""    | 插件路径。如果是硬编码注册的插件不需要填 |
| aimrt.plugin.plugins[i].options       | map           | 可选  | -     | 传递给插件的初始化配置，具体内容在各个插件章节介绍 |


&emsp;&emsp;以下是一个简单的示例：
```yaml
aimrt:
  plugin: # 【可选】插件配置根节点
    plugins: # 【可选】各个插件的配置
      - name: xxx_plugin # 【必选】插件名称
        path: ./libmqtt_plugin.so # 【可选】插件路径。如果是硬编码注册的插件不需要填
        options: # 【可选】传递给插件的初始化配置，具体内容在各个插件章节介绍
          xxx_key: xxx_val
          yyy_key: yyy_val
```

&emsp;&emsp;`aimrt.plugin`使用注意点如下：
- `plugins[i].name`用于配置插件名称。不允许出现重复的插件名称。
- 如果配置了`plugins[i].path`，AimRT框架会从该路径下加载对应的插件动态库文件。如果使用者基于App模式硬编码注册插件，则不需要配置此项。
- `plugins[i].options`是AimRT传递给插件的初始化参数，这部分配置格式由各个插件定义，请参考对应插件的文档。



## `aimrt.main_thread`：主线程执行器


&emsp;&emsp;`aimrt.main_thread`配置项用于配置主线程。配置项说明如下：

| 节点                                  | 类型          | 是否可选| 默认值 | 作用 |
| ----                                  | ----          | ----  | ----  | ---- |
| aimrt.main_thread                     | map           | 可选  | -     | 主线程配置根节点 |
| aimrt.main_thread.name                | string        | 可选  | "aimrt_main"    | 主线程名称 |
| aimrt.main_thread.thread_sched_policy | string        | 可选  | ""    | 线程调度策略 |
| aimrt.main_thread.thread_bind_cpu     | unsigned int array | 可选 | [] | 绑核配置 |

&emsp;&emsp;以下是一个简单的示例：
```yaml
aimrt:
  main_thread: # 【可选】主线程配置根节点
    name: main_thread # 【可选】主线程名称
    thread_sched_policy: SCHED_FIFO:80 # 【可选】线程调度策略
    thread_bind_cpu: [0, 1] # 【可选】绑核配置
```

&emsp;&emsp;`aimrt.main_thread`使用注意点如下：
- `name`配置了主线程名称，在实现时调用了操作系统的一些API。如果操作系统不支持，则此项配置无效。
- `thread_sched_policy`配置了线程调度策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setschedparam`这个API来配置。支持的方式包括：`SCHED_OTHER`、`SCHED_FIFO:xx`、`SCHED_RR:xx`。`xx`为该模式下的权重值。详细的解释请参考[pthread_setschedparam官方文档](https://man7.org/linux/man-pages/man3/pthread_setschedparam.3.html)。
- `thread_bind_cpu`配置了绑核策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setaffinity_np`这个API来配置，直接在数组中配置CPU ID即可。参考[pthread_setaffinity_np官方文档](https://man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html)。

## `aimrt.executor`：执行器

&emsp;&emsp;`aimrt.executor`配置项用于配置执行器。配置项说明如下：

| 节点                                  | 类型      | 是否可选| 默认值 | 作用 |
| ----                                  | ----      | ----  | ----  | ---- |
| aimrt.executor                        | map       | 可选  | -     | 执行器配置根节点 |
| aimrt.executor.executors              | array     | 可选  | []    | 执行器列表 |
| aimrt.executor.executors[i].name      | string    | 必选  | ""    | 执行器名称 |
| aimrt.executor.executors[i].type      | string    | 必选  | ""    | 执行器类型 |
| aimrt.executor.executors[i].options   | map       | 可选  | -     | 具体执行器的配置 |

&emsp;&emsp;以下是一个简单的示例：
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

&emsp;&emsp;`aimrt.executor`的配置说明如下：
- `executors[i].name`表示执行器名称。不允许出现重复的执行器名称。
- `executors[i].type`表示执行器类型。AimRT官方提供了几种执行器类型，部分插件也提供了一些执行器类型。
- `executors[i].options`是AimRT传递给各个执行器的初始化参数，这部分配置格式由各个执行器类型定义，请参考对应执行器类型的文档。

### `asio_thread`执行器

&emsp;&emsp;`asio_thread`执行器是一种基于[Asio库](https://github.com/chriskohlhoff/asio)实现的执行器，是一种线程池，可以手动设置线程数，此外它还支持定时调度。其所有的配置项如下：

| 节点                          | 类型                  | 是否可选| 默认值 | 作用 |
| ----                          | ----                  | ----  | ----  | ---- |
| thread_num                    | unsigned int          | 可选  | 1     | 线程数 |
| threads_ched_policy           | string                | 可选  | ""    | 线程调度策略 |
| thread_bind_cpu               | unsigned int array    | 可选  | []    | 绑核配置 |
| timeout_alarm_threshold_us    | unsigned int          | 可选  | 1000000 | 调度超时告警阈值，单位：微秒 |


&emsp;&emsp;以下是一个简单的示例：
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

&emsp;&emsp;使用注意点如下：
- `thread_num`配置了线程数，默认为1。当线程数配置为 1 时为线程安全执行器，否则是线程不安全的。
- `thread_sched_policy`配置了线程调度策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setschedparam`这个API来配置。支持的方式包括：`SCHED_OTHER`、`SCHED_FIFO:xx`、`SCHED_RR:xx`。`xx`为该模式下的权重值。详细的解释请参考[pthread_setschedparam官方文档](https://man7.org/linux/man-pages/man3/pthread_setschedparam.3.html)。
- `thread_bind_cpu`配置了绑核策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setaffinity_np`这个API来配置，直接在数组中配置CPU ID即可。参考[pthread_setaffinity_np官方文档](https://man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html)。
- `timeout_alarm_threshold_us`配置了一个调度超时告警的阈值。当进行定时调度时，如果CPU负载太重、或队列中任务太多，导致超过设定的时间才调度到，则会打印一个告警日志。



### `asio_strand`执行器

&emsp;&emsp;`asio_strand`执行器是一种依附于`asio_thread`执行器的伪执行器，基于Asio库的strand实现。它不能独立存在，并不拥有实际的线程，它在运行过程中会将任务交给绑定的`asio_thread`执行器来实际执行。但是它保证线程安全，也支持定时调度。其所有的配置项如下：

| 节点                              | 类型          | 是否可选| 默认值 | 作用 |
| ----                              | ----          | ----  | ----  | ---- |
| bind_asio_thread_executor_name    | string        | 必选  | ""     | 绑定的asio_thread执行器名称 |
| timeout_alarm_threshold_us        | unsigned int  | 可选  | 1000000 | 调度超时告警阈值，单位：微秒 |


&emsp;&emsp;以下是一个简单的示例：
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

&emsp;&emsp;使用注意点如下：
- 通过`bind_asio_thread_executor_name`配置项来绑定`asio_thread`类型的执行器。如果指定名称的执行器不存在、或不是`asio_thread`类型，则会在初始化时抛出异常。
- `timeout_alarm_threshold_us`配置了一个调度超时告警的阈值。当进行定时调度时，如果CPU负载太重、或队列中任务太多，导致超过设定的时间才调度到，则会打印一个告警日志。


### `tbb_thread`执行器

&emsp;&emsp;`tbb_thread`是一种基于[oneTBB库](https://github.com/oneapi-src/oneTBB)的无锁并发队列实现的高性能无锁线程池，可以手动设置线程数，但它不支持定时调度。其所有的配置项如下：

| 节点                          | 类型                  | 是否可选| 默认值 | 作用 |
| ----                          | ----                  | ----  | ----  | ---- |
| thread_num                    | unsigned int          | 可选  | 1     | 线程数 |
| threads_ched_policy           | string                | 可选  | ""    | 线程调度策略 |
| thread_bind_cpu               | unsigned int array    | 可选  | []    | 绑核配置 |
| timeout_alarm_threshold_us    | unsigned int          | 可选  | 1000000 | 调度超时告警阈值，单位：微秒 |



&emsp;&emsp;以下是一个简单的示例：
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


&emsp;&emsp;使用注意点如下：
- `thread_num`配置了线程数，默认为1。当线程数配置为 1 时为线程安全执行器，否则是线程不安全的。
- `thread_sched_policy`配置了线程调度策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setschedparam`这个API来配置。支持的方式包括：`SCHED_OTHER`、`SCHED_FIFO:xx`、`SCHED_RR:xx`。`xx`为该模式下的权重值。详细的解释请参考[pthread_setschedparam官方文档](https://man7.org/linux/man-pages/man3/pthread_setschedparam.3.html)。
- `thread_bind_cpu`配置了绑核策略，通过调用操作系统的API来实现。目前仅在Linux下支持，在其他操作系统上此配置无效。
  - 在Linux下通过调用`pthread_setaffinity_np`这个API来配置，直接在数组中配置CPU ID即可。参考[pthread_setaffinity_np官方文档](https://man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html)。
- `timeout_alarm_threshold_us`配置了一个调度超时告警的阈值。当进行定时调度时，如果CPU负载太重、或队列中任务太多，导致超过设定的时间才调度到，则会打印一个告警日志。



### `time_wheel`执行器

&emsp;&emsp;`time_wheel`执行器是一种基于时间轮实现的执行器，一般用于有大量定时任务、且对定时精度要求不高的场景，例如RPC超时处理。它会启动一个单独的线程跑时间轮，同时也支持将具体的任务投递到其他执行器中执行。其所有的配置项如下：


| 节点                  | 类型                  | 是否可选| 默认值 | 作用 |
| ----                  | ----                  | ----  | ----  | ---- |
| bind_executor         | string                | 可选  | ""    | 绑定的执行器 |
| dt_us                 | unsigned int          | 可选  | 1000  | 时间轮tick的间隔，单位：微秒 |
| wheel_size            | unsigned int array    | 可选  | [1000, 600]    | 各个时间轮的大小 |
| threads_ched_policy   | string                | 可选  | ""    | 时间轮线程的调度策略 |
| thread_bind_cpu       | unsigned int array    | 可选  | []    | 时间轮线程的绑核配置 |


&emsp;&emsp;以下是一个简单的示例：
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

&emsp;&emsp;使用注意点如下：
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

&emsp;&emsp;`aimrt.log`配置项用于配置日志。配置项说明如下：

| 节点                          | 类型      | 是否可选| 默认值 | 作用 |
| ----                          | ----      | ----  | ----  | ---- |
| aimrt.log                     | map       | 可选  | -     | 日志配置根节点 |
| aimrt.log.core_lvl            | string    | 可选  | "Info" | 框架日志等级 |
| aimrt.log.default_module_lvl  | string    | 可选  | "Info" | 默认的模块日志等级 |
| aimrt.log.backends            | array     | 可选  | ""    | 日志后端列表 |
| aimrt.log.backends[i].type    | string    | 必选  | ""    | 日志后端类型 |
| aimrt.log.backends[i].options | map       | 可选  | -     | 具体日志后端的配置 |

&emsp;&emsp;其中，日志等级可选项包括以下几种（不区分大小写）：
- Trace
- Debug
- Info
- Warn
- Error
- Fatal
- Off

&emsp;&emsp;以下是一个简单的示例：
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

&emsp;&emsp;`aimrt.log`的配置说明如下：
- `core_lvl`表示AimRT运行时内核的日志等级，内核日志一般设为Info级别即可。
- `default_module_lvl`默认的模块日志等级。
- `backends[i].type`是日志后端的类型。AimRT官方提供了几种日志后端，部分插件也提供了一些日志后端类型。
- `backends[i].options`是AimRT传递给各个日志后端的初始化参数，这部分配置格式由各个日志后端类型定义，请参考对应日志后端类型的文档。


### `console`控制台日志后端


&emsp;&emsp;`console`日志后端用于将日志打印到控制台上。其所有的配置项如下：


| 节点                  | 类型              | 是否可选| 默认值 | 作用 |
| ----                  | ----              | ----  | ----  | ---- |
| color                 | bool              | 可选  | true  | 是否要彩色打印 |
| log_executor_name     | string            | 可选  | ""    | 日志执行器。默认使用主线程 |


&emsp;&emsp;以下是一个简单的示例：
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
          log_executor_name: test_log_executor.log # 【可选】日志执行器。默认使用主线程
```

&emsp;&emsp;使用注意点如下：
- `color`配置了是否要彩色打印。此项配置有可能在一些操作系统不支持。
- `log_executor_name`配置了日志执行器。要求日志执行器是线程安全的，如果没有配置，则默认使用主线程打印日志。


### `rotate_file`滚动文件日志后端

&emsp;&emsp;`rotate_file`日志后端用于将日志打印到文件中。其所有的配置项如下：


| 节点                  | 类型              | 是否可选| 默认值 | 作用 |
| ----                  | ----              | ----  | ----  | ---- |
| path                  | string            | 必选  | "./log"  | 日志文件存放目录 |
| filename              | string            | 必选  | "aimrt.log" | 日志文件基础名称 |
| max_file_size_m       | unsigned int      | 可选  | 16    | 日志文件最大尺寸，单位：Mb |
| max_file_num          | unsigned int      | 可选  | 100   | 日志文件最大数量。0表示无上限 |
| log_executor_name     | string            | 可选  | ""    | 日志执行器。默认使用主线程 |


&emsp;&emsp;以下是一个简单的示例：
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
```

&emsp;&emsp;使用注意点如下：
- `path`配置了日志文件的存放路径。如果路径不存在，则会创建一个。如果创建失败，会抛异常。
- `filename`配置了日志文件基础名称。
- 当单个日志文件尺寸超过`max_file_size_m`配置的大小后，就会新建一个日志文件，同时将老的日志文件重命名，加上`_x`这样的后缀。
- 当日志文件数量超过`max_file_num`配置的值后，就会将最老的日志文件删除。如果配置为0，则表示永远不会删除。

## `aimrt.rpc`：RPC

***TODO待完善***

&emsp;&emsp;`aimrt.rpc`配置项用于配置RPC功能。配置项说明如下：

| 节点                                          | 类型      | 是否可选| 默认值 | 作用 |
| ----                                          | ----      | ----  | ----  | ---- |
| aimrt.rpc                                     | map       | 可选  | -     | RPC配置根节点 |
| aimrt.rpc.backends                            | array     | 可选  | []    | RPC后端列表 |
| aimrt.rpc.backends[i].type                    | string    | 必选  | ""    | RPC后端类型 |
| aimrt.rpc.backends[i].options                 | map       | 可选  | -     | 具体RPC后端的配置 |
| aimrt.rpc.clients_options                     | array     | 可选  | ""    | RPC Client配置 |
| aimrt.rpc.clients_options[i].func_name        | string    | 必选  | ""    | RPC Client名称，支持正则表达式 |
| aimrt.rpc.clients_options[i].enable_backends  | string array | 必选  | [] | RPC Client允许使用的RPC后端列表 |
| aimrt.rpc.servers_options                     | array     | 可选  | ""    | RPC Server配置 |
| aimrt.rpc.servers_options[i].func_name        | string    | 必选  | ""    | RPC Server名称，支持正则表达式 |
| aimrt.rpc.servers_options[i].enable_backends  | string array | 必选  | [] | RPC Server允许使用的RPC后端列表 |


&emsp;&emsp;以下是一个简单的示例：
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



## `aimrt.channel`：Channel


***TODO待完善***

&emsp;&emsp;`aimrt.channel`配置项用于配置Channel功能。配置项说明如下：

| 节点                                              | 类型      | 是否可选| 默认值 | 作用 |
| ----                                              | ----      | ----  | ----  | ---- |
| aimrt.channel                                     | map       | 可选  | -     | Channel配置根节点 |
| aimrt.channel.backends                            | array     | 可选  | []    | Channel后端列表 |
| aimrt.channel.backends[i].type                    | string    | 必选  | ""    | Channel后端类型 |
| aimrt.channel.backends[i].options                 | map       | 可选  | -     | 具体Channel后端的配置 |
| aimrt.channel.pub_topics_options                  | array     | 可选  | ""    | Channel Pub Topic配置 |
| aimrt.channel.pub_topics_options[i].topic_name    | string    | 必选  | ""    | Channel Pub Topic名称，支持正则表达式 |
| aimrt.channel.pub_topics_options[i].enable_backends | string array | 必选  | [] | Channel Pub Topic允许使用的Channel后端列表 |
| aimrt.channel.sub_topics_options                  | array     | 可选  | ""    | Channel Sub Topic配置 |
| aimrt.channel.sub_topics_options[i].topic_name    | string    | 必选  | ""    | Channel Sub Topic名称，支持正则表达式 |
| aimrt.channel.sub_topics_options[i].enable_backends | string array | 必选  | [] | Channel Sub Topic允许使用的Channel后端列表 |


&emsp;&emsp;以下是一个简单的示例：
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


## `aimrt.parameter`：参数


***TODO：parameter 暂无可配置的参数***



