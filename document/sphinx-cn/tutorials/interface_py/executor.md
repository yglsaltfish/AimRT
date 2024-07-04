# Executor


## ExecutorManagerRef：执行器句柄

执行器的具体概念请参考[执行器-CPP接口](../interface_cpp/executor.md)中的介绍。此章节仅介绍具体的语法。模块可以通过调用`CoreRef`句柄的`GetExecutorManager()`接口，获取`ExecutorManagerRef`句柄，其中提供了一个简单的获取Executor的接口：
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

