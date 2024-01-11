include(FetchContent)

message(STATUS "get pybind11 ...")

set(pybind11_DOWNLOAD_URL
    "https://github.com/pybind/pybind11/archive/v2.11.1.tar.gz"
    CACHE STRING "")

FetchContent_Declare(
  pybind11
  URL ${pybind11_DOWNLOAD_URL}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  OVERRIDE_FIND_PACKAGE)

FetchContent_GetProperties(pybind11)
if(NOT pybind11_POPULATED)
  FetchContent_MakeAvailable(pybind11)
endif()
