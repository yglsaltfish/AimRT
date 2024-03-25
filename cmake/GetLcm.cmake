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
  FetchContent_Populate(lcm)

  set(LCM_ENABLE_TESTS
      OFF
      CACHE BOOL "")

  set(LCM_ENABLE_EXAMPLES
      OFF
      CACHE BOOL "")

  set(LCM_ENABLE_PYTHON
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

  set(LCM_INSTALL_M4MACROS
      OFF
      CACHE BOOL "")

  set(LCM_INSTALL_PKGCONFIG
      OFF
      CACHE BOOL "")

  file(READ ${lcm_SOURCE_DIR}/CMakeLists.txt TMP_VAR)
  string(REPLACE "add_subdirectory(lcmgen)" "" TMP_VAR "${TMP_VAR}")
  string(REPLACE "add_subdirectory(lcm-logger)" "" TMP_VAR "${TMP_VAR}")
  string(REPLACE "add_subdirectory(docs)" "" TMP_VAR "${TMP_VAR}")
  string(REPLACE "include(lcm-cmake/install.cmake)" "" TMP_VAR "${TMP_VAR}")
  string(REPLACE "include(lcm-cmake/cpack.cmake)" "" TMP_VAR "${TMP_VAR}")
  file(WRITE ${lcm_SOURCE_DIR}/CMakeLists.txt "${TMP_VAR}")

  add_subdirectory(${lcm_SOURCE_DIR} ${lcm_BINARY_DIR})

  if(TARGET lcm-static)
    add_library(lcm::lcm-static ALIAS lcm-static)
  endif()

endif()

# import targets：
# lcm::lcm-static
