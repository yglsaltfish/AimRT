
# 录播插件




## 插件概述


**record_playback_plugin**用于对 channel 数据进行录制和播放。插件支持加载独立的type_support_pkg，并支持立即模式、信号触发模式等多种工作模式。



插件的配置项如下：

| 节点                              | 类型          | 是否可选| 默认值  | 作用 |
| ----                              | ----          | ----  | ----      | ---- |
| type_support_pkgs                 | array         | 可选  | []        | type support 包配置 |
| type_support_pkgs[i].path         | string        | 必选  | ""        | type support 包的路径 |
| record_actions                    | array         | 可选  | []        | 录制动作配置 |
| record_actions[i].name            | string        | 必选  | ""        | 动作名称 |
| record_actions[i].options         | map           | 必选  | -         | 动作选项 |
| record_actions[i].options.bag_path        | string        | 必选  | ""        | 录制包存放的路径 |
| record_actions[i].options.max_bag_size_m  | unsigned int  | 可选  | 2048      | 录制包 db 最大尺寸，单位 MB |
| record_actions[i].options.max_bag_num     | unsigned int  | 可选  | 0         | 录制包的最大个数，超出后将删除最早的包。0 表示无限大 |
| record_actions[i].options.mode            | string        | 必选  | ""        | 录制模式，不区分大小写，立即模式：imd，信号触发模式：signal |
| record_actions[i].options.max_preparation_duration_s  | unsigned int  | 可选  | 0      | 最大提前数据预备时间，仅 signal 模式下生效 |
| record_actions[i].options.executor        | string        | 必选  | ""        | 录制使用的执行器，要求必须是线程安全的 |
| record_actions[i].options.topic_meta_list | array        | 可选  | []        | 要录制的 topic 和类型 |
| record_actions[i].options.topic_meta_list[j].topic_name   | string        | 必选  | ""        | 要录制的 topic |
| record_actions[i].options.topic_meta_list[j].msg_type     | string        | 必选  | ""        | 要录制的消息类型 |
| record_actions[i].options.topic_meta_list[j].serialization_type     | string        | 可选  | ""        | 录制时使用的序列化类型，不填则使用该消息类型的默认序列化类型 |
| playback_actions                  | array         | 可选  | []        | 播放动作配置 |
| playback_actions[i].name          | string        | 必选  | ""        | 动作名称 |
| playback_actions[i].options       | map           | 必选  | -         | 动作选项 |
| playback_actions[i].options.bag_path      | string        | 必选  | ""        | 录制包的路径 |
| playback_actions[i].options.mode          | string        | 必选  | ""        | 播放模式，不区分大小写，立即模式：imd，信号触发模式：signal |
| playback_actions[i].options.executor      | string        | 必选  | ""        | 播放使用的执行器，要求必须支持 time schedule |
| playback_actions[i].options.skip_duration_s   | unsigned int  | 可选  | 0      | 播放时跳过的时间，仅 imd 模式下生效 |
| playback_actions[i].options.play_duration_s   | unsigned int  | 可选  | 0      | 播放时长，仅 imd 模式下生效。0 表示播完整个包 |
| playback_actions[i].options.topic_meta_list   | array         | 可选  | []    | 要播放的 topic 和类型，必须要在录制包中存在。如果不填，则默认播放所有 |
| playback_actions[i].options.topic_meta_list[j].topic_name   | string        | 必选  | ""        | 要播放的 topic |
| playback_actions[i].options.topic_meta_list[j].msg_type     | string        | 必选  | ""        | 要播放的消息类型 |


此外，插件还注册了一个基于protobuf协议定义的RPC：`RecordPlaybackService`，提供了信号触发模式下的一些接口，协议文件为[record_playback.proto](https://code.agibot.com/agibot_aima/aimrt/-/blob/main/src/protocols/plugins/record_playback_plugin/record_playback.proto)。请注意，**record_playback_plugin**没有提供任何通信后端，因此本插件一般要搭配其他通信插件的RPC通信后端一块使用，例如[net_plugin](./net_plugin.md)中的http RPC后端。



以下是一个信号触发录制功能的简单示例配置：
```yaml
aimrt:
  plugin:
    plugins:
      - name: net_plugin
        path: ./libaimrt_net_plugin.so
        options:
          thread_num: 4
          http_options:
            listen_ip: 127.0.0.1
            listen_port: 50080
      - name: record_playback_plugin
        path: ./libaimrt_record_playback_plugin.so
        options:
          type_support_pkgs:
            - path: ./libexample_event_type_support_pkg.so
          record_actions:
            - name: my_signal_record
              options:
                bag_path: ./bag
                max_bag_size_m: 2048
                mode: signal # imd/signal
                max_preparation_duration_s: 10 # Effective only in signal mode
                executor: record_thread # require thread safe!
                topic_meta_list:
                  - topic_name: test_topic
                    msg_type: pb:aimrt.protocols.example.ExampleEventMsg
                    serialization_type: pb # optional
  executor:
    executors:
      - name: record_thread
        type: simple_thread
  channel:
    # ...
  rpc:
    backends:
      - type: http
    servers_options:
      - func_name: "(pb:/aimrt.protocols.record_playback_plugin.*)"
        enable_backends: [http]
```

