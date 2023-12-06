set_namespace()

if(AIMRT_BUILD_EXAMPLES AND AIMRT_BUILD_WITH_PROTOBUF)
  add_subdirectory(example)
endif()

if(AIMRT_BUILD_EXAMPLES AND AIMRT_BUILD_WITH_ROS2)
  add_subdirectory(example_ros2)
endif()

if(AIMRT_BUILD_WITH_PROTOBUF)
  add_subdirectory(common)
  add_subdirectory(geometry)
  add_subdirectory(sensor)
  add_subdirectory(visualization)
endif()

if(AIMRT_BUILD_WITH_ROS2)
  add_subdirectory(sensor_ros2)
endif()

if(AIMRT_BUILD_WITH_PROTOBUF AND AIMRT_BUILD_TIME_MANIPULATOR_PLUGIN)
  add_subdirectory(time_manipulator_plugin)
endif()
