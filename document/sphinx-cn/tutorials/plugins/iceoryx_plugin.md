
# Iceoryx插件


## 插件概述

**iceoryx_plugin**是一个基于iceoryx的高性能插件，其提供了一种零拷贝的方式来在不同的进程之间共享内存，从而实现低延迟的数据传输，此插件为AimRT提供了以下组件：
- `iceoryx`类型Channel后端

## 适用场景
- 基于共享内存的本机通信
- 低时延、高频率数据传输

## 配置

插件的配置项如下：

| 节点             | 类型      | 是否可选| 默认值 | 作用 |
| :----:           | :----:   | :----: | :----:| :----: |
| shm_init_size    | int      | 可选  | 1024 (bytes)   | 初始向共享内存池中租赁的共享内存大小 |



以下是一个简单的示例：
```yaml
aimrt:
  plugin:
    plugins:
      - name: iceoryx_plugin
        path: ./libaimrt_iceoryx_plugin.so
        options:
          shm_init_size: 2048 # 【可选】初始向共享内存池中租赁的共享内存大小， 单位bytes
```


关于**iceoryx_plugin**的配置，使用注意点如下：
- 使用该插件前请确保；相关依赖安装：
  - 64-bit hardware (e.g. x86_64 or aarch64; 32-bit hardware might work, but is not supported)
  - CMake, 3.16 or later
  - One of the following compilers:
    - GCC, 8.3 or later
    - Clang, 9.0 or later
    - MSVC, part of Visual Studio 2019 or later
  - **libacl, 2.2 or later. Only for Linux & QNX.** 
  ```bash
  sudo apt-get install libacl1-dev
  ```
  - optional, ncurses, 6.2 or later. Required by introspection tool (only for Linux, QNX and MacOS).


- `shm_init_size`表示初始分配的共享内存大小，默认1kbytes。注意，在实际运行过程中，可能数据的尺寸大于所分配的共享内存大小，AimRT采用一种动态扩容机制，会以2倍的增长速率进行扩容，直到满足数据需求，之后的共享内存申请大小会按照扩容后的大小进行申请。


- **iceoryx_plugin**插件基于[iceoryx](https://github.com/eclipse-iceoryx/iceoryx)封装，在使用时，一定要先启动Roudi，用户在可终端输入如下命令启动：
  ```bash
  <Your Build Path>/iox-roudi [配置选项]
  ```
  可以看到，用户可以在命令行输入配置选项，这是一个可选项，如果用户不输入，则使用默认配置。关于Roudi的配置，具体可以参考[iceoryx overview.md](https://github.com/eclipse-iceoryx/iceoryx/blob/main/doc/website/getting-started/overview.md)。需要强调的是，配置选项中可以通过toml配置文件对共享内存池进行配置，用户在使用前一定要确保开辟的共享内存池的大小适配实际的硬件资源，下面是一个例子：
  ```toml
  # Adapt this config to your needs and rename it to e.g. roudi_config.toml
  # Reference to https://github.com/eclipse-iceoryx/iceoryx/blob/main/iceoryx_posh/etc/iceoryx/roudi_config_example.toml

  [general]
  version = 1

  [[segment]]

  [[segment.mempool]]
  size = 128
  count = 10000

  [[segment.mempool]]
  size = 1024
  count = 5000

  [[segment.mempool]]
  size = 16384
  count = 1000

  [[segment.mempool]]
  size = 131072
  count = 200
  ```
  ```bash
   <Your Build Path>/iox-roudi --config-file=/<Your Toml Config Path>/iox_cfg.toml
  ```


## `iceoryx` Channel后端


`iceoryx`类型的Channel后端是**iceoryx_plugin**中提供的一种Channel后端，用于通过iceoryx提供的共享内存方式来发布和订阅消息。其所有的配置项如下：

以下是一个简单的发布端的示例：
```yaml
aimrt:
  plugin:
    plugins:
      - name: iceoryx_plugin # 【必选】插件名
        path: ./libaimrt_iceoryx_plugin.so
        options:
          shm_init_size: 2048 # 【可选】初始向共享内存池中租赁的共享内存大小， 单位bytes
  executor:
    executors:
      - name: work_thread_pool
        type: asio_thread
        options:
          thread_num: 2
  channel:
    backends:
      - type: iceoryx
    pub_topics_options:
      - topic_name: "(.*)" 
        enable_backends: [iceoryx]

```

以下则是一个简单的订阅端的示例：
```yaml
aimrt:
  plugin:
    plugins:
      - name: iceoryx_plugin # 【必选】插件名
        path: ./libaimrt_iceoryx_plugin.so
        options:
          shm_init_size: 2048 # 【可选】初始向共享内存池中租赁的共享内存大小， 单位bytes
  executor:
  channel:
    backends:
      - type: iceoryx
    sub_topics_options:
      - topic_name: "(.*)"
        enable_backends: [iceoryx]
```



在AimRT发布端发布数据到订阅端这个链路上，Iceoryx数据包格式整体分4段：
- 数据包长度， 20字节
- 序列化类型，一般是`pb`或`json`
- context区
  - context数量，1字节，最大255个context
  - context_1 key, 2字节长度 + 数据区
  - context_2 key, 2字节长度 + 数据区
  - ...
- 数据

```
| msg len [20 byte]
| n(0~255) [1 byte] | content type [n byte]
| context num [1 byte]
| context_1 key size [2 byte] | context_1 key data [key_1_size byte]
| context_1 val size [2 byte] | context_1 val data [val_1_size byte]
| context_2 key size [2 byte] | context_2 key data [key_2_size byte]
| context_2 val size [2 byte] | context_2 val data [val_2_size byte]
| ...
| msg data [len - 1 - n byte] |
```
