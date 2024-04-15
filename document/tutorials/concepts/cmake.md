
# AimRT中的CMake

&emsp;&emsp;AimRT使用Modern CMake进行构建，并基于CMake Fetchcontent获取依赖。

## Modern CMake
&emsp;&emsp;Modern CMake是目前主流C++项目的常用构建方式，其使用面向对象的构建思路，引入了Target、属性等概念，将包括依赖在内的各个参数信息全部封装起来，大大简化了依赖管理，使构建大型系统更有条理、更加轻松。`AimRT`框架使用Modern CMake进行构建，每个叶子文件夹是一个CMake Target。库之间相互引用时能自动引用下层级的所有依赖。

&emsp;&emsp;关于Modern CMake详细使用方式此处不做赘述，您可以参考一些其他教程：
- [More Modern CMake](https://hsf-training.github.io/hsf-training-cmake-webpage/aio/index.html)
- [Effective Modern CMake](https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1)
- [An Introduction to Modern CMake](https://cliutils.gitlab.io/modern-cmake/)
- [cmake官方文档add_library页面](https://cmake.org/cmake/help/latest/command/add_library.html)
- [cmake学习例子集合](https://github.com/ttroy50/cmake-examples)


## AimRT的CMake选项
&emsp;&emsp;AimRT框架由其interface层、runtime主体，加上多个插件、工具共同组成，在构建时可以通过配置CMake选项，选择其中一部分或全部进行构建。详细的CMake选项列表见AimRT根CMakeLists.txt文件。


## AimRT的第三方依赖管理策略
&emsp;&emsp;当您准备从源码引用/构建AimRT时，您需要了解AimRT第三方依赖管理策略：
- AimRT使用CMake FetchContent拉取依赖，关于CMake FetchContent的详细使用方式，请参考[CMake官方文档](https://cmake.org/cmake/help/latest/module/FetchContent.html)。
- AimRT默认从各个第三方依赖的官方下载地址进行下载，如果你想通过自定义的下载地址下载这些依赖，可以参考`cmake/GetXXX.cmake`中的代码，在构建时传入`-DXXX_DOWNLOAD_URL`参数，将下载url修改为您自定义的地址。
- 如果您的构建环境无法连接外部网络，你也可以离线的下载这些依赖，然后参考`cmake/GetXXX.cmake`中的代码，在构建时传入`-DXXX_LOCAL_SOURCE`参数，将依赖寻找地址转为您指定的本地地址。
- 如果以上方式还不满足您对依赖管理的自定义需求，您也可以直接自定义`cmake/GetXXX.cmake`中的代码，只要引入满足AimRT构建所需的CMake Target即可。
- 请注意，AimRT仅验证了默认参数中配置的各个依赖的版本，如果您需要升级或降级这些依赖的版本，请自行保证这些第三方依赖的兼容性和稳定性。

