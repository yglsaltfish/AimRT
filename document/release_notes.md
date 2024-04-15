
# ReleaseNotes

## v0.6.0

&emsp;&emsp;修改日志：

- 修复了mqtt插件的一些问题。
- 修复了topic/rpc规则配置的问题，现在是以第一个命中的规则为准，命中后就不会再管后续规则了。
- 去除了各个插件的单独的enable配置。
- 调整了example体系。
- 优化了main_executor的性能。
- 提供了rpc/channel统一的backend开关，参考example。
