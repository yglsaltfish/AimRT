set_root_namespace("aimrt")

add_subdirectory(common)
add_subdirectory(interface)
add_subdirectory(tools)
add_subdirectory(protocols)

if(AIMRT_BUILD_RUNTIME)
  add_subdirectory(runtime)
  add_subdirectory(plugins)
endif()

if(AIMRT_BUILD_RUNTIME AND AIMRT_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
