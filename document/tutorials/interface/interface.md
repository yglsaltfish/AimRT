
# 接口概述

[TOC]

&emsp;&emsp;本文档介绍使用AimRT接口时的一些基本须知。

## 逻辑实现接口与部署运行接口

&emsp;&emsp;在AimRT中，一个重要是思想是将**逻辑实现**与实际**部署运行**分离。用户在开发业务逻辑时不需要关心最后部署运行的方式，在最终部署运行时发生的一些变化也不需要修改业务逻辑代码。例如，当用户在编写一个RPC Client和Server端逻辑时，只需要知道Client发起的请求Server端一定会收到，而不需要关心client端和Server端会部署在哪，也不用关心实际运行时底层数据会通过什么方式通信。等到部署时，使用者才需要决定Client端和Server是部署在端上还是云上/是部署在一台物理节点还是多台物理节点上，然后再根据部署情况选择合适的底层通信方式，例如是共享内存通信还是通过网络通信。

&emsp;&emsp;因此，对应与这套设计思想，AimRT中广义的接口分为两大部分：
- 用户在开发业务逻辑时所需要知晓的接口，包括如何打日志、如何读取配置、如何调用RPC等。
- 用户在部署运行时所需要知晓的接口/配置，包括如何集成编译各个模块、如何选定底层通信方式等。注意：这里所需要关心的不仅包括基于C++/Python等语言的代码形式的接口，也包括配置文件的配置项。


&emsp;&emsp;这两部分接口的详细文档目录：
- 逻辑实现接口
  - [CPP模块接口](interface/cpp_module.md)
  - [Python模块接口](interface/py_module.md)
- 部署运行接口
  - [CPP运行时接口](interface/cpp_runtime.md)
  - [Python运行时接口](interface/py_runtime.md)
  - [配置](interface/cfg.md)

## AimRT运行时生命周期

&emsp;&emsp;AimRT框架在运行时依次有三大阶段：Initialize、Start、Shutdown。这三个阶段的意义以及对应阶段做的事情如下：
- Initialize阶段：
  - 初始化AimRT框架；
  - 初步初始化业务，申请好业务所需的AimRT框架中的资源；
  - 在主线程中依次完成所有的初始化，不会开其他线程，所有代码是线程安全的；
  - 部分接口或资源不能在此阶段使用；
- Start阶段：
  - 完全初始化业务；
  - 启动业务相关逻辑；
  - 可以开始使用AimRT中的所有资源，如发起RPC、将任务投递到线程池等；
  - 在主线程中依次启动各个业务，业务可以再调度到多线程环境中；
- Shutdown阶段：
  - 通常由ctrl-c等信号触发；
  - 优雅停止业务；
  - 优雅停止AimRT框架；
  - 在主线程中阻塞的等待所有业务逻辑结束；

&emsp;&emsp;简而言之，就是一些申请AimRT资源的操作，只能放在Initialize阶段去做，以此来保证在业务运行期间，AimRT框架不会有新的资源申请操作，也不会有太多的锁操作，保证Start阶段的效率和稳定性。

&emsp;&emsp;但是要注意，这里的Initialize阶段只是指AimRT的Initialize，一些业务的Initialize可能需要调用AimRT框架在Start阶段才开放的一些接口才能完成。因此业务的Initialize可能要在AimRT框架Start阶段后才能完成，不能和AimRT框架的Initialize混为一谈。

