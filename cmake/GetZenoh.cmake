include(FetchContent)

message(STATUS "Getting zenohc...")

# check rust compiler
execute_process(
  COMMAND rustc --version
  RESULT_VARIABLE rustc_result
  OUTPUT_VARIABLE rustc_output
  ERROR_QUIET)

if(rustc_result EQUAL 0)
  message(STATUS "Rust compiler (rustc) found: ${rustc_output}")
else()
  message(FATAL_ERROR "Rust compiler (rustc) not found. Please install Rust environment refering to https://www.rust-lang.org/tools/install.")
endif()

# fetch zenoh-c
set(zenohc_DOWNLOAD_URL
    "https://github.com/eclipse-zenoh/zenoh-c/archive/refs/tags/1.0.0.6.tar.gz"
    CACHE STRING "")

if(${zenohc_LOCAL_SOURCE})
  FetchContent_Declare(
    zenohc
    SOURCE_DIR ${zenohc_LOCAL_SOURCE}
    OVERRIDE_FIND_PACKAGE)
else()
  FetchContent_Declare(
    zenohc
    URL ${zenohc_DOWNLOAD_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    OVERRIDE_FIND_PACKAGE)
endif()

FetchContent_GetProperties(zenohc)
if(NOT zenohc_POPULATED)
  FetchContent_MakeAvailable(zenohc)
endif()
