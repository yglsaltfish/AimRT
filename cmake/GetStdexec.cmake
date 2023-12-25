include(FetchContent)

message(STATUS "get stdexec ...")

set(stdexec_DOWNLOAD_URL
    "https://github.com/NVIDIA/stdexec/archive/nvhpc-23.09.rc4.tar.gz"
    CACHE STRING "")

FetchContent_Declare(
  stdexec
  URL ${stdexec_DOWNLOAD_URL}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  OVERRIDE_FIND_PACKAGE)

FetchContent_GetProperties(stdexec)
if(NOT stdexec_POPULATED)
  set(STDEXEC_ENABLE_IO_URING_TESTS
      OFF
      CACHE BOOL "")

  set(STDEXEC_BUILD_EXAMPLES
      OFF
      CACHE BOOL "")

  set(STDEXEC_BUILD_TESTS
      OFF
      CACHE BOOL "")

  FetchContent_MakeAvailable(stdexec)
endif()

# import targets:
# STDEXEC::stdexec
# STDEXEC::nvexec
# STDEXEC::tbbexec
