set_namespace()

add_subdirectory(util)

if(AIMRT_BUILD_RUNTIME)
  add_subdirectory(net)
endif()

if(AIMRT_BUILD_WITH_ROS2)
  add_subdirectory(ros2_util)
endif()
