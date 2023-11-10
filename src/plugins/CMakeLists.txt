set_namespace()

if(AIMRT_BUILD_NET_PLUGIN)
  add_subdirectory(net_plugin)
endif()

if(AIMRT_BUILD_SM_PLUGIN AND CMAKE_SYSTEM_NAME MATCHES "Linux")
  add_subdirectory(sm_plugin)
endif()

if(AIMRT_BUILD_LCM_PLUGIN)
  add_subdirectory(lcm_plugin)
endif()

if(AIMRT_BUILD_WITH_ROS2 AND AIMRT_BUILD_ROS2_PLUGIN)
  add_subdirectory(ros2_plugin)
endif()

if(AIMRT_BUILD_RECORD_PLAYBACK_PLUGIN)
  add_subdirectory(record_playback_plugin)
endif()
