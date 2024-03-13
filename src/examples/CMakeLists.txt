set_namespace()

add_subdirectory(example_helloworld)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  add_subdirectory(example_real_time)
endif()

if(AIMRT_BUILD_WITH_PROTOBUF)
  add_subdirectory(example_normal_channel)
  add_subdirectory(example_normal_rpc)
endif()

if(AIMRT_BUILD_WITH_PROTOBUF AND AIMRT_BUILD_NET_PLUGIN)
  add_subdirectory(example_tcp_channel)
  add_subdirectory(example_udp_channel)
  add_subdirectory(example_http_channel)
  add_subdirectory(example_http_rpc)
endif()

if(AIMRT_BUILD_WITH_ROS2 AND AIMRT_BUILD_ROS2_PLUGIN)
  add_subdirectory(example_ros2_channel)
  add_subdirectory(example_ros2_rpc)
endif()

if(AIMRT_BUILD_WITH_PROTOBUF
   AND AIMRT_BUILD_SM_PLUGIN
   AND CMAKE_SYSTEM_NAME MATCHES "Linux")
  add_subdirectory(example_sm_channel)
endif()

if(AIMRT_BUILD_WITH_PROTOBUF
   AND AIMRT_BUILD_LCM_PLUGIN
   AND CMAKE_SYSTEM_NAME MATCHES "Linux")
  add_subdirectory(example_lcm_channel)
endif()

if(AIMRT_BUILD_WITH_PROTOBUF AND AIMRT_BUILD_MQTT_PLUGIN)
  add_subdirectory(example_mqtt_channel)
  add_subdirectory(example_mqtt_rpc)
endif()

if(AIMRT_BUILD_RECORD_PLAYBACK_PLUGIN)
  add_subdirectory(example_record_playback)
endif()

if(AIMRT_BUILD_WITH_PROTOBUF
   AND AIMRT_BUILD_NET_PLUGIN
   AND AIMRT_BUILD_TIME_MANIPULATOR_PLUGIN)
  add_subdirectory(example_time_manipulator)
endif()

if(AIMRT_BUILD_WITH_PROTOBUF
   AND AIMRT_BUILD_NET_PLUGIN
   AND AIMRT_BUILD_PARAMETER_PLUGIN)
  add_subdirectory(example_parameter)
endif()

# python interface
if(AIMRT_BUILD_PYTHON_RUNTIME)
  add_subdirectory(example_helloworld_py)
endif()

if(AIMRT_BUILD_PYTHON_RUNTIME AND AIMRT_BUILD_WITH_PROTOBUF)
  add_subdirectory(example_normal_channel_py)
  add_subdirectory(example_normal_rpc_py)
endif()
