# aimrt.guard_thread

## 守护线程执行器配置概述


`aimrt.guard_thread`配置项用于配置守护线程。其中的细节配置项说明如下：

| 节点                | 类型          | 是否可选| 默认值 | 作用 |
| ----                | ----          | ----  | ----  | ---- |
| name                | string        | 可选  | "aimrt_guard"    | 守护线程名称 |
| thread_sched_policy | string        | 可选  | ""    | 线程调度策略 |
| thread_bind_cpu     | unsigned int array | 可选 | [] | 绑核配置 |

以下是一个简单的示例：
```yaml
aimrt:
  guard_thread: # 【可选】守护线程配置根节点
    name: guard_thread # 【可选】守护线程名称
    thread_sched_policy: SCHED_FIFO:80 # 【可选】线程调度策略
    thread_bind_cpu: [0, 1] # 【可选】绑核配置
```


`aimrt.guard_thread`使用注意点如下：
- `name`配置了守护线程名称，在实现时调用了操作系统的一些 API。如果操作系统不支持，则此项配置无效。
- `thread_sched_policy`和`thread_bind_cpu`参考[配置概述](./common.md)中线程绑核配置的说明。