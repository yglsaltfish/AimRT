set_namespace()

if(AIMRT_BUILD_WITH_PROTOBUF)
  add_subdirectory(protoc_plugin_cpp_gen_aimrt_rpc)
  add_subdirectory(protoc_plugin_py_gen_aimrt_rpc)
endif()

if(AIMRT_BUILD_WITH_ROS2)
  add_subdirectory(ros2_py_gen_aimrt_rpc)
endif()

if(AIMRT_BUILD_CLI_TOOLS)
  add_subdirectory(aimrt_cli)
endif()
