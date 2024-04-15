
# HelloWorld CPP

本章将以一个HelloWorld的Demo来介绍如何建立一个最基本的AimRT CPP工程。

## 确保本地环境符合要求

&emsp;&emsp;请先确保本地安装有AimRT，并且本地的编译环境符合要求，具体请参考[安装](installation.md)中对编译环境的要求。


## 创建目录结构，并添加基本的文件

&emsp;&emsp;参照以下目录结构，创建相关文件：

```
TODO
```

## 实现一个简单的`Module`

&emsp;&emsp;通过以下代码可以实现一个简单的`Module`：
```cpp
#include "aimrt_module_cpp_interface/module_base.h"

class HelloWorldModule : public aimrt::ModuleBase {
 public:
  HelloWorldModule() = default;
  ~HelloWorldModule() override = default;

  // 模块基本信息
  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "HelloWorldModule"};
  }

  // 初始化，框架在加载模块后会调用模块的初始化方法，传入CoreRef，作为模块调用框架功能的句柄
  bool Initialize(aimrt::CoreRef core) noexcept override {
    core_ = core;
    AIMRT_HL_INFO(core_.GetLogger(), "Init succeeded.");
    return true;
  }
  
  // 启动模块，在初始化后框架会调用模块的Start方法
  bool Start() noexcept override {
    AIMRT_HL_INFO(core_.GetLogger(), "Start succeeded.");
    return true;
  }

  // 关闭，框架在结束运行时会调用模块的关闭方法
  void Shutdown() noexcept override {
    AIMRT_HL_INFO(core_.GetLogger(), "Shutdown succeeded.");
  }

 private:
  aimrt::CoreRef core_; // 模块应将初始化时传入的CoreRef存储下来
};
```

## 编译一个简单的`Pkg`
&emsp;&emsp;通过以下代码可以实现一个简单的`Pkg`：

```cpp
#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "helloworld_module/helloworld_module.h"

// 核心是创建一个<模块名称, 模块创建方法>的表
static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"HelloWorldModule", []() -> aimrt::ModuleBase* { return HelloWorldModule(); }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
```

## 启动一个`AimRT`进程

&emsp;&emsp;启动一个`AimRT`进程需要一个配置文件。框架使用yaml作为基本配置文件格式。参照如下yaml代码作为配置文件：
```yaml
aimrt:
  configurator:
    temp_cfg_path: ./cfg/tmp # 生成的临时模块配置文件存放路径
  log: # log配置
    core_lvl: INFO # 内核日志等级，可选项：Trace/Debug/Info/Warn/Error/Fatal/Off，不区分大小写
    default_module_lvl: INFO # 模块默认日志等级
    backends: # 日志backends
      - type: console # 控制台日志
        options:
          color: true # 是否彩色打印
      - type: rotate_file # 文件日志
        options:
          path: ./log # 日志文件路径
          filename: examples_cpp_helloworld.log # 日志文件名称
          max_file_size_m: 4 # 日志文件最大尺寸，单位m
          max_file_num: 10 # 最大日志文件数量，0代表无限
  module: # 模块配置
    pkgs: # 要加载的动态库配置
      - path: ./libhelloworld_pkg.so # so/dll地址
        disable_module: [] # 此动态库中要屏蔽的模块名称。默认全部加载
    modules: # 模块
      - name: HelloWorldModule # 模块Name接口返回的名称
        log_lvl: INFO # 模块日志级别
```

&emsp;&emsp;这个配置文件中的其他内容将在后续章节中介绍。这里关注`aimrt.module`节点，这个节点配置了要加载的模块包动态库路径，以及各个模块的基本配置。我们在这里配置加载刚刚编译的`helloworld_pkg`模块包动态库，加载其中的`HelloWorldModule`模块。

&emsp;&emsp;将以上配置文件保存为`test_cfg.yaml`，并确保要加载的模块包处于正确的路径下，然后执行以下命令启动`AimRT`进程：
```shell
aimrt_main --cfg_file_path=/path/to/test_cfg.yaml
```

&emsp;&emsp;启动后即可通过日志观察模块运行情况。启动后按下`ctrl + c`即可停止进程。其他启动参数可以通过`aimrt_main --help`查看。

