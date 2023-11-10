include(FetchContent)

message(STATUS "get libunifex ...")

set(libunifex_DOWNLOAD_URL
    "https://github.com/facebookexperimental/libunifex/archive/591ec09e7d51858ad05be979d4034574215f5971.tar.gz"
    CACHE STRING "")

FetchContent_Declare(
  libunifex
  URL ${libunifex_DOWNLOAD_URL}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  OVERRIDE_FIND_PACKAGE)

FetchContent_GetProperties(libunifex)
if(NOT libunifex_POPULATED)
  FetchContent_MakeAvailable(libunifex)

  add_library(unifex::unifex ALIAS unifex)
endif()

# import targets:
# unifex::unifex
