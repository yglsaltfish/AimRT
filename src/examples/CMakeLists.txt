set_namespace()

add_subdirectory(cpp)

if(AIMRT_BUILD_PYTHON_RUNTIME)
  add_subdirectory(py)
endif()

add_subdirectory(plugins)
