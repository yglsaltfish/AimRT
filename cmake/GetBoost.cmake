include(FetchContent)

message(STATUS "get boost ...")

set(boost_DOWNLOAD_URL
    "https://github.com/boostorg/boost/releases/download/boost-1.82.0/boost-1.82.0.tar.xz"
    CACHE STRING "")

FetchContent_Declare(
  boost
  URL ${boost_DOWNLOAD_URL}
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  OVERRIDE_FIND_PACKAGE)

FetchContent_GetProperties(boost)
if(NOT boost_POPULATED)
  set(BOOST_INCLUDE_LIBRARIES beast asio)

  set(Boost_USE_STATIC_LIBS
      ON
      CACHE BOOL "")

  FetchContent_MakeAvailable(boost)
endif()
