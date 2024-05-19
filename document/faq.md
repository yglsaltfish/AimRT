
# FAQ

[TOC]


## AimRT对比ROS2有何优势和劣势？

**AimRT体系对比ROS2体系的优势**：
- 【轻量】AimRT非常轻量，约3万行代码，依赖很少，不挑操作系统，易于部署集成。ROS2较为冗重，超过数十万行代码，依赖非常多，编译体系用自己的一套，官方只在少数几个操作系统上维护，部署集成很麻烦。
- 【设计更现代】AimRT在资源管控、异步编程、部署配置等许多方面都吸取了很多现代、且主流的设计思想，可保证在未来多年之内不会过时。
- 【支持更多底层通信方式】AimRT和ROS2都不做底层通信，通信层交给插件。但ROS2只支持DDS，主要用在端上相互通信。AimRT支持的插件更多更灵活，包括ROS2后端、Http后端、Mqtt后端等，覆盖常见的端、云通信场景。
- 【对云、AI领域支持较好】AimRT从上层通信接口、底层通信方式上都对云、AI领域支持的更好，例如支持Protobuf做为接口协议，兼容HTTP、Mqtt等。而ROS2主要用于较底层的机器人运控、Slam领域。
- 【兼容ROS2】AimRT支持ROS2协议和ROS2插件，可以和原生ROS2模块通信，兼容所有ROS2生态模块。


**AimRT体系对比ROS2体系的劣势**：
- 【生态少】ROS2的周边生态组件非常多，很多算法在开发时都需要依赖这些组件。AimRT兼容ROS2能在一定程度上缓解这个劣势，但完全消除这个劣势还需要较长时间的积累。
- 【部分语法比较新】AimRT提供了基于C++20协程的异步接口，这部分比较新的C++特性需要一定的学习成本。
- 【文档、测试比较欠缺】AimRT还处于快速迭代阶段，文档还在不断完善，各种测试也在不断补充，但要达到比较完善全面的阶段还需要时间。


## AimRT如何兼容ROS2？

&emsp;&emsp;AimRT对ROS2的兼容包括2个方面：
- 兼容ROS2的协议，代表业务逻辑层的兼容；
- 能够和原生ROS2节点通信，代表运行时的兼容；

&emsp;&emsp;AimRT官方支持ROS2协议，只需要打开指定的CMake Option并引用特定的CMake Target即可，然后在逻辑层面就可以非常自然的使用ROS2作为RPC、Channel的消息类型。但此时，AimRT节点与其他AimRT节点之间的底层通信可能并不是基于ROS2，这将根据配置的RPC、Channel后端决定，可能是Mqtt、Tcp或者Http等。

&emsp;&emsp;AimRT官方还支持ROS2（Humble）插件，此插件提供了ROS2的RPC和Channel后端，对外通过ROS2进行通信，在外界看来这个AimRT节点就是一个ROS2 Node。它们将基于以下原则工作：
- 如果上层逻辑层使用ROS2协议，那么它们会将消息采用原生ROS2的形式发送出去，外部ROS2节点直接基于原生的ROS2协议即可。
- 如果上层逻辑层使用的是非ROS2协议，例如Protobuf，那么它们会将消息序列化到一个特殊的ROS2协议中，外部ROS2节点需要订阅这个特殊的ROS2协议，并从中反序列化得到正真的Protobuf消息。

详情请参考[Module接口-Cpp版本](./tutorials/interface/cpp_module.md)和[ROS2插件](./tutorials/plugins/ros2_plugin.md)文档。
