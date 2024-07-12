get_target_property(py_files aimrt::runtime::python_runtime PY_FILES)
file(GLOB_RECURSE gencode_file ${CMAKE_CURRENT_SOURCE_DIR}/../protoc_plugin_py_gen_aimrt_py_rpc/*.py)
list(APPEND py_files ${gencode_file})
add_custom_target(
  copy_python_runtime ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/aimrt_py_pkg/aimrt_py/
  COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aimrt::runtime::python_runtime> ${CMAKE_BINARY_DIR}/aimrt_py_pkg/aimrt_py/
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${py_files} ${CMAKE_BINARY_DIR}/aimrt_py_pkg/aimrt_py/)

add_custom_target(
  create_python_pkg ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/aimrt_py_pkg
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml ${CMAKE_CURRENT_SOURCE_DIR}/setup.py ${CMAKE_BINARY_DIR}/aimrt_py_pkg/
  COMMAND ${Python3_EXECUTABLE} setup.py sdist bdist_wheel
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/aimrt_py_pkg)

function(copy_plugin_if_enabled PLUGIN_NAME)
  string(TOUPPER ${PLUGIN_NAME} PLUGIN_BUILD_FLAG)
  set(PLUGIN_BUILD_FLAG "AIMRT_BUILD_${PLUGIN_BUILD_FLAG}_PLUGIN")

  if(${PLUGIN_BUILD_FLAG})
    message(STATUS "Copying ${PLUGIN_NAME} plugin to aimrt_py_pkg")
    add_custom_target(
      copy_${PLUGIN_NAME}_plugin ALL
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/aimrt_py_pkg/aimrt_py/
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aimrt::plugins::${PLUGIN_NAME}_plugin> ${CMAKE_BINARY_DIR}/aimrt_py_pkg/aimrt_py/)
    add_dependencies(create_python_pkg copy_${PLUGIN_NAME}_plugin)
  endif()
endfunction()

# record_playback plugin not included
set(PLUGIN_LIST
    net
    sm
    ros2
    lcm
    mqtt
    time_manipulator
    parameter
    log_control)

foreach(PLUGIN_NAME ${PLUGIN_LIST})
  copy_plugin_if_enabled(${PLUGIN_NAME})
endforeach()
