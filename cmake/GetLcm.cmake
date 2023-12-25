include(FetchContent)

message(STATUS "get lcm ...")

set(lcm_DOWNLOAD_URL
    "https://github.com/lcm-proj/lcm/archive/refs/tags/v1.5.0.tar.gz"
    CACHE STRING "")

FetchContent_Declare(
  lcm
  URL ${lcm_DOWNLOAD_URL}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  OVERRIDE_FIND_PACKAGE)

FetchContent_GetProperties(lcm)
if(NOT lcm_POPULATED)
  set(LCM_ENABLE_TESTS
      OFF
      CACHE BOOL "")

  set(LCM_ENABLE_EXAMPLES
      OFF
      CACHE BOOL "")

  set(LCM_ENABLE_JAVA
      OFF
      CACHE BOOL "")

  set(LCM_ENABLE_LUA
      OFF
      CACHE BOOL "")

  set(LCM_ENABLE_GO
      OFF
      CACHE BOOL "")

  FetchContent_MakeAvailable(lcm)
  add_library(lcm::lcm ALIAS lcm)
endif()

# import targets：
# lcm::lcm
