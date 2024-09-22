# logger examples


## logger

一个最基本的 cpp logger 示例，演示内容包括：
- 如何创建一个 LoggerModule 并使用AimRT自带的 Log 功能；
- 不同的日志级别演示示例；


核心代码：
- [logger_module.cc](./module/logger_module/logger_module.cc)
- [pkg_main.cc](./pkg/logger_pkg/pkg_main.cc)


配置文件：
- [examples_cpp_logger_cfg.yaml](./install/linux/bin/cfg/examples_cpp_logger_cfg.yaml)


运行方式：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 直接运行 build 目录下`start_examples_cpp_logger.sh`脚本启动进程；
- 键入`ctrl-c`停止进程；


说明：
- 此示例创建了一个 `LoggerModule`，会在`Initialize`时读取配置并打印出来，同时还会在其 `Initialize`、`Start`、`Shutdown`的阶段各打印一行日志；
- 此示例将 `LoggerModule` 集成到 `liblogger_pkg` 中，并在配置文件中加载此 Pkg；
- 可以在启动后观察控制台打印出来的日志，了解不同日志级别的输出效果,AimRT支持的日志等级如下:
 
| 日志级别 | 含义                                                                               | 颜色                                   |
| -------- | ---------------------------------------------------------------------------------- | -------------------------------------- |
| Trace    | 追踪信息，最详细的日志级别，通常用于开发阶段，用于追踪程序执行过程中的每一个细节。 | <span style="color:white">白色</span>  |
| Debug    | 调试信息，用于开发和调试过程中，记录一些开发者需要关注的变量和状态信息。           | <span style="color:blue">蓝色</span>   |
| Info     | 信息性日志，记录程序正常运行的重要信息，如启动、停止等事件。                       | <span style="color:green">绿色</span>  |
| Warn     | 警告信息，表示可能出现的问题，但并不影响程序的继续运行，通常提示需要注意。         | <span style="color:yellow">黄色</span> |
| Error    | 错误信息，表示程序中发生了错误，可能导致部分功能不可用，但程序仍在运行。           | <span style="color:purple">紫色</span> |
| Fatal    | 致命错误，表示程序遇到严重问题，必须立即停止运行。                                 | <span style="color:red">红色</span>    |
| Off      | 关闭日志记录，不记录任何日志信息。                                                 | -                                      |



  
## logger rotate file

一个最基本的 cpp logger rotate  示例，演示内容包括：
- 如何将生成的日志保存到指定目录下；
-  如何使用 Log rotate 功能并了解其配置项；

核心代码：
- [logger_module.cc](./module/logger_module/logger_module.cc)
- [pkg_main.cc](./pkg/logger_pkg/pkg_main.cc)


配置文件：
- [examples_cpp_logger_rotate_file_cfg.yaml](./install/linux/bin/cfg/examples_cpp_logger_rotate_file_cfg.yaml)
可以看到在该配置文件中多了如下配置：
```yaml
      - type: rotate_file       #指明日志记录的类型为“rotate_file”
        options:
          path: ./log           #日志保存目标目录
          filename: examples_cpp_logger_rotate_file.log #日志名
          max_file_size_m: 4    #定义了单个日志文件的最大大小，单位为MB。这里指定为4MB，意味着当日志文件达到或超过这个大小时，就会触发轮替
          max_file_num: 10      #指定了要保留的最大日志文件数量。在这个配置中，最多会保留10个日志文件，包括当前正在使用的那个文件。
```

运行方式：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 直接运行 build 目录下`start_examples_cpp_logger.sh`脚本启动进程；
- 键入`ctrl-c`停止进程；


说明：
- 此示例创建了一个 `LoggerModule`，会在`Initialize`时读取配置并打印出来，同时还会在其 `Initialize`、`Start`、`Shutdown`的阶段各打印一行日志；
- 此示例将 `LoggerModule` 集成到 `liblogger_pkg` 中，并在配置文件中加载此 Pkg；
- 可以在配置文件指定的目录中 "./log" 看到生成的日志文件 "examples_cpp_logger_rotate_file.log" ；


## logger specify executor

一个最基本的 cpp logger executor  示例，演示内容包括：
- 如何使用指定的执行器运行日志；

核心代码：
- [logger_module.cc](./module/logger_module/logger_module.cc)
- [pkg_main.cc](./pkg/logger_pkg/pkg_main.cc)


配置文件：
- [examples_cpp_logger_specify_executor_cfg.yaml](./install/linux/bin/cfg/examples_cpp_logger_specify_executor_cfg.yaml)
可以看到在该配置文件中多了如下配置：
```yaml
  log:
    core_lvl: INFO # Trace/Debug/Info/Warn/Error/Fatal/Off
    backends:
      - type: console
        options:
          log_executor_name: log_executor  #指定日志执行器名称，这里要和executors中列举的执行器列表匹配
  executor:
    executors: #执行器列表
      - name: work_executor
        type: simple_thread
      - name: log_executor  #执行器名称（可自定义）
        type: simple_thread #执行器类型（类型具体参考AimRT使用手册执行器部分）
```

运行方式：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 直接运行 build 目录下`start_examples_cpp_logger.sh`脚本启动进程；
- 键入`ctrl-c`停止进程；


说明：
- 此示例创建了一个 `LoggerModule`，会在`Initialize`时读取配置并打印出来，同时还会在其 `Initialize`、`Start`、`Shutdown`的阶段各打印一行日志；
- 此示例将 `LoggerModule` 集成到 `liblogger_pkg` 中，并在配置文件中加载此 Pkg；
- 该日志模块将会在配置文件指定的执器上运行；



## logger bench

一个最基本的 cpp logger benchmark 示例，演示内容包括：
- 如何创建一个 LoggerBenchModule；
- 如何跑 AimRT的日志性能测试；

核心代码：
- [logger_bench_module.cc](./module/logger_bench_module/logger_bench_module.cc)
- [pkg_main.cc](./pkg/logger_pkg/pkg_main.cc)


配置文件：
- [examples_cpp_logger_bench_cfg.yaml](./install/linux/bin/cfg/examples_cpp_logger_bench_cfg.yaml)

可以看到在该配置文件中多了如下配置：
```yaml
LoggerBenchModule:
  log_data_size: [64, 256, 1024, 4096] #要测试的日志数据大小列表，单位为字节
  log_bench_num: 5000 #每组测试跑多少次日志记录
```
请注意这里我们在配置文件的 module/pkg中的 enable_modules以及 module/modules/name中都修改为了 LoggerBenchModule

运行方式：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 直接运行 build 目录下`start_examples_cpp_logger.sh`脚本启动进程；
- 键入`ctrl-c`停止进程；


说明：
- 此示例创建了一个 `LoggerBenchModule`，会在`Initialize`时读取配置并打印出来，同时还会在其 `Initialize`、`Start`、`Shutdown`的阶段各打印一行日志；
- 此示例将 `LoggerModule` 集成到 `liblogger_pkg` 中，并在配置文件中加载此 Pkg；
- 该日志模块将会在配置文件指定的执器上运行；
