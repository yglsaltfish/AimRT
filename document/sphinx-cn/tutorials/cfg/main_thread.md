# aimrt.main_thread

## 主线程执行器配置概述


`aimrt.main_thread`配置项用于配置主线程。其中的细节配置项说明如下：

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
