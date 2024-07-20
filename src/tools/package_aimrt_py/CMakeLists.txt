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
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml ${CMAKE_CURRENT_SOURCE_DIR}/setup.py ${CMAKE_CURRENT_SOURCE_DIR}/MANIFEST.in
          ${CMAKE_BINARY_DIR}/aimrt_py_pkg/
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/VERSION ${CMAKE_SOURCE_DIR}/README.md ${CMAKE_SOURCE_DIR}/LICENSE ${CMAKE_BINARY_DIR}/aimrt_py_pkg/
  COMMAND ${Python3_EXECUTABLE} -m build
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/aimrt_py_pkg)

include(PluginDetectTool)
get_subdirectories(${CMAKE_SOURCE_DIR}/src/plugins PLUGIN_LIST)
message(STATUS "Detected plugins: ${PLUGIN_LIST}")

function(copy_plugin_if_enabled PLUGIN_NAME)
  string(TOUPPER ${PLUGIN_NAME} PLUGIN_BUILD_FLAG)
  set(PLUGIN_BUILD_FLAG "AIMRT_BUILD_${PLUGIN_BUILD_FLAG}")

  if(${PLUGIN_BUILD_FLAG})
    message(STATUS "Copying ${PLUGIN_NAME} plugin to aimrt_py_pkg")
    add_custom_target(
      copy_${PLUGIN_NAME} ALL
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/aimrt_py_pkg/aimrt_py/
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aimrt::plugins::${PLUGIN_NAME}> ${CMAKE_BINARY_DIR}/aimrt_py_pkg/aimrt_py/)
    add_dependencies(create_python_pkg copy_${PLUGIN_NAME})
  endif()
endfunction()

foreach(PLUGIN_NAME ${PLUGIN_LIST})
  if(TARGET ${PLUGIN_NAME})
    copy_plugin_if_enabled(${PLUGIN_NAME})
  endif()
endforeach()
