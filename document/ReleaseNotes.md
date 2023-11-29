
# ReleaseNotes

## v0.3.0
&emsp;&emsp;***AimRT v0.3.0***版本是AimRT的第一个正式版本。

- v0.3.0版本功能上已经相对完备，未来一段时间内在接口层面将不会有太大变动。但测试、文档上还有一些完善空间。
- v0.3.0版本在未来一段时间仍将不断发布小版本号，主要是完善文档、测试、优化代码、修复使用过程中新发现的Bug等，但不会有大的重构。
- v0.3.0版本的Rpc部分目前还是不太成熟。但鉴于目前大部分使用场景都是基于Channel，因此此问题影响可控。Rpc功能将在后续继续优化。
- AimRT短期内仍然会由软件组这边的人员帮助各算法同事接入使用。
- 下一阶段的建设重点将集中在AimRT的周边生态 上，包括上位机、开发工具、仿真平台等。


## v0.3.1

&emsp;&emsp;修改日志：

- 完善了一些接口的命名空间。
- 做了一些代码命名、代码格式上的优化。
- 修复net插件中`http rpc backend`的一些问题。
- 将协程的内部支持库由libunifex改为stdexec。接口上有一些小改动，主要在async_scope上。
- 优化了aimrt_cli。
- 支持TBB作为一种新的执行器，名称为`tbb_executor`。主线程执行器的实现也换为TBB类型。原来的asio实现的执行器仍然保留，名称由`thread`改为`asio_thread`。
- 将一些std::map/std::set改为了std::unordered_map/std::unordered_set。

