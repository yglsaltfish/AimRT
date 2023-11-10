# Get the current folder name
string(REGEX REPLACE ".*/\(.*\)" "\\1" CUR_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Set namespace
set_namespace()

# module
add_subdirectory(module/ros2_publisher_module)
add_subdirectory(module/ros2_subscriber_module)

# pkg
add_subdirectory(pkg/ros2_channel_alpha_pkg)
add_subdirectory(pkg/ros2_channel_beta_pkg)

# misc
add_subdirectory(assistant/native_ros2_channel_publisher)
add_subdirectory(assistant/native_ros2_channel_subscriber)

# install
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CUR_INSTALL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/install/linux)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(CUR_INSTALL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/install/win)
else()
  message(FATAL_ERROR "Unsupport os")
endif()

install(DIRECTORY ${CUR_INSTALL_SOURCE_DIR}/bin DESTINATION ./)

# build all
get_namespace(CUR_SUPERIOR_NAMESPACE)
string(REPLACE "::" "_" CUR_SUPERIOR_NAMESPACE_UNDERLINE ${CUR_SUPERIOR_NAMESPACE})
add_custom_target(
  ${CUR_SUPERIOR_NAMESPACE_UNDERLINE}_${CUR_DIR}_build_all ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CUR_INSTALL_SOURCE_DIR}/bin $<TARGET_FILE_DIR:aimrt::runtime::main>
  DEPENDS aimrt::runtime::main
          ${CUR_SUPERIOR_NAMESPACE}::${CUR_DIR}::ros2_channel_alpha_pkg
          ${CUR_SUPERIOR_NAMESPACE}::${CUR_DIR}::ros2_channel_beta_pkg
          ${CUR_SUPERIOR_NAMESPACE}::${CUR_DIR}::native_ros2_channel_publisher
          ${CUR_SUPERIOR_NAMESPACE}::${CUR_DIR}::native_ros2_channel_subscriber)
