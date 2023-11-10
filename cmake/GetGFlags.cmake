include(FetchContent)

message(STATUS "get gflags ...")

set(gflags_DOWNLOAD_URL
    "https://github.com/gflags/gflags/archive/v2.2.2.tar.gz"
    CACHE STRING "")

FetchContent_Declare(
  gflags
  URL ${gflags_DOWNLOAD_URL}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  OVERRIDE_FIND_PACKAGE)

FetchContent_GetProperties(gflags)
if(NOT gflags_POPULATED)
  set(BUILD_TESTING
      OFF
      CACHE BOOL "")

  FetchContent_MakeAvailable(gflags)
endif()

# import targets:
# gflags::gflags
