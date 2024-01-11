# Get the current folder name
string(REGEX REPLACE ".*/\(.*\)" "\\1" CUR_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Set namespace
set_namespace()

# module
add_subdirectory(module/normal_publisher_module)
add_subdirectory(module/normal_subscriber_module)
add_subdirectory(module/benchmark_publisher_module)
add_subdirectory(module/benchmark_subscriber_module)

# pkg
add_subdirectory(pkg/normal_channel_alpha_pkg)
add_subdirectory(pkg/normal_channel_beta_pkg)
add_subdirectory(pkg/normal_channel_omega_pkg)
add_subdirectory(pkg/normal_channel_delta_pkg)

# install
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CUR_INSTALL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/install/linux)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(CUR_INSTALL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/install/win)
else()
  message(FATAL_ERROR "Unsupport os")
endif()

install(
  DIRECTORY ${CUR_INSTALL_SOURCE_DIR}/bin
  DESTINATION ./
  USE_SOURCE_PERMISSIONS)

# build all
get_namespace(CUR_SUPERIOR_NAMESPACE)
string(REPLACE "::" "_" CUR_SUPERIOR_NAMESPACE_UNDERLINE ${CUR_SUPERIOR_NAMESPACE})
add_custom_target(
  ${CUR_SUPERIOR_NAMESPACE_UNDERLINE}_${CUR_DIR}_build_all ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CUR_INSTALL_SOURCE_DIR}/bin $<TARGET_FILE_DIR:aimrt::runtime::main>
  DEPENDS aimrt::runtime::main
          ${CUR_SUPERIOR_NAMESPACE}::${CUR_DIR}::normal_channel_alpha_pkg
          ${CUR_SUPERIOR_NAMESPACE}::${CUR_DIR}::normal_channel_beta_pkg
          ${CUR_SUPERIOR_NAMESPACE}::${CUR_DIR}::normal_channel_omega_pkg
          ${CUR_SUPERIOR_NAMESPACE}::${CUR_DIR}::normal_channel_delta_pkg)
