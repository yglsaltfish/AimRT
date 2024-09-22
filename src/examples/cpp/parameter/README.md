# parameter examples


## parameter

一个最基本的 cpp parameter 示例，演示内容包括：
- 如何创建一个 ParameterModule 并通过 SetParameter和 GetParameter方法设置和获取参数的值；


核心代码：
- [parameter_module.cc](./module/parameter_module/parameter_module.cc)
- [pkg_main.cc](./pkg/parameter_pkg/pkg_main.cc)


配置文件：
- [examples_cpp_parameter_cfg.yaml](./install/linux/bin/cfg/examples_cpp_parameter_cfg.yaml)


运行方式：
- 开启 `AIMRT_BUILD_EXAMPLES` 选项编译 AimRT；
- 直接运行 build 目录下`start_examples_cpp_parameter.sh`脚本启动进程；
- 键入`ctrl-c`停止进程；


说明：
- 此示例创建了一个 `ParameterModule`，会在`Initialize`时读取配置并打印出来，同时还会在其 `Start`的阶段循设置和读取参数的值并打印在终端上；
- 此示例将 `ParameterModule` 集成到 `libparameter_pkg` 中，并在配置文件中加载此 Pkg；
