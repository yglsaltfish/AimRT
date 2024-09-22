# executor examples


## executor

一个最基本的 cpp executor 示例，演示内容包括：
- 如何创建一个 ExecutorModule；
- 如何通过配置文件对执行器进行设置；
- 展示两个启动执行器的方法： Execute和 ExecuteAfter；


核心代码：
- [executor_module.cc](./module/executor_module/executor_module.cc)
- [pkg_main.cc](./pkg/executor_pkg/pkg_main.cc)


配置文件：
- [examples_cpp_executor_cfg.yaml](./install/linux/bin/cfg/examples_cpp_executor_cfg.yaml)


运行方式：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 直接运行 build 目录下`start_examples_cpp_executor.sh`脚本启动进程；
- 键入`ctrl-c`停止进程；


说明：
- 此示例创建了一个 `ExecutorModule`，会在`Initialize`时读取配置并打印出来，可以看到在打印出的配置信息的 "Executor List" 中显示了用户在配置文件中注册的执行器
- 在 `Start`的阶段会依次使用所注册的执行器运行具体的逻辑任务：
  - work_executor：在终端打印一句 "This is a simple task"；
  - thread_safe_executor：开启多个线程同时递增n的数值共10000次，并打印出最终n的值，由于是线程安全，故会打印 "Value of n is 10000"；
  - time_schedule_executor：定时间隔1 打印当前的循环次数；
- 此示例将 `ExecutorModule` 集成到 `libexecutor_pkg` 中，并在配置文件中加载此 Pkg；



## executor co

一个最基本的 cpp executor co 示例，演示内容包括：
- 如何以协程的方式启动执行器；

核心代码：
- [executor_co_module.cc](./module/executor_co_module/executor_co_module.cc)
- [pkg_main.cc](./pkg/executor_pkg/pkg_main.cc)


配置文件：
- [examples_cpp_executor_co_cfg.yaml](./install/linux/bin/cfg/examples_cpp_executor_co_cfg.yaml)


运行方式：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 直接运行 build 目录下`start_examples_cpp_executor_co.sh`脚本启动进程；
- 键入`ctrl-c`停止进程；


说明：
- 此示例创建了一个 `ExecutorCoModule`，会在`Initialize`时读取配置并打印出来，可以看到在打印出的配置信息的 "Executor List" 中显示了用户在配置文件中注册的执行器
- 在 `Start`的阶段会依次使用所注册的执行器运行具体的逻辑任务：
  - work_executor：在终端打印一句 "This is a simple task"；
  - thread_safe_executor：开启多个线程同时递增n的数值共10000次，并打印出最终n的值，由于是线程安全，故会打印 "Value of n is 10000"；
  - time_schedule_executor：定时间隔1 打印当前的循环次数；
- 此示例将 `ExecutorCoModule` 集成到 `libexecutor_pkg` 中，并在配置文件中加载此 Pkg；

## executor co loop

一个最基本的 cpp executor co loop 示例，演示内容包括：
- 如何以协程的方式启动执行器；

核心代码：
- [executor_co_loop_module.cc](./module/executor_co_loop_module/executor_co_loop_module.cc)
- [pkg_main.cc](./pkg/executor_pkg/pkg_main.cc)


配置文件：
- [examples_cpp_executor_co_loop_cfg.yaml](./install/linux/bin/cfg/examples_cpp_executor_co_loop_cfg.yaml)


运行方式：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 直接运行 build 目录下`start_examples_cpp_executor_co_loop.sh`脚本启动进程；
- 键入`ctrl-c`停止进程；


说明：
- 此示例创建了一个 `ExecutorCoLoopModule`，会在`Initialize`时读取配置并打印出来，紧接着，在 `Start`的阶段循会以1s的定时间隔， 打印当前的循环次数；
- 此示例将 `ExecutorCoLoopModule` 集成到 `libexecutor_pkg` 中，并在配置文件中加载此 Pkg；

## executor real time

一个最基本的 cpp executor real time 示例，演示内容包括：
- 如何通过配置文件设置CPU调度策：SCHED_OTHER 、 SCHED_FIFO 和 SCHED_RR；
- 如何通过配置文件为执行器绑定到指定CPU；

核心代码：
- [real_time_module.cc](./module/real_time_module/real_time_module.cc)
- [pkg_main.cc](./pkg/executor_pkg/pkg_main.cc)


配置文件：
- [examples_cpp_executor_real_time_cfg.yaml](./install/linux/bin/cfg/examples_cpp_executor_real_time_cfg.yaml)


运行方式：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 直接运行 build 目录下`start_examples_cpp_executor_real_time.sh`脚本启动进程；
- 键入`ctrl-c`停止进程；


说明：
- 此示例创建了一个 `RealTimeModule`，会在`Initialize`时读取配置并打印出来；
- 在 `Start`的阶会依次以三种不同调度策略异步开启三个执行器，并在终端打印其循环次数、调度策略，当前使用的CPU等信息：
- 此示例将 `RealTimeModule` 集成到 `libexecutor_pkg` 中，并在配置文件中加载此 Pkg；

