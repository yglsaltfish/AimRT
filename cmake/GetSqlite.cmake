include(FetchContent)

message(STATUS "get sqlite ...")

set(sqlite_DOWNLOAD_URL
    "https://www.sqlite.org/2023/sqlite-amalgamation-3420000.zip"
    CACHE STRING "")

# sqlite version: https://www.sqlite.org/chronology.html
FetchContent_Declare(
  sqlite
  URL ${sqlite_DOWNLOAD_URL}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_GetProperties(sqlite)
if(NOT sqlite_POPULATED)
  FetchContent_Populate(sqlite)

  # sqlite lib
  add_library(libsqlite)
  add_library(sqlite::libsqlite ALIAS libsqlite)

  file(GLOB head_files ${sqlite_SOURCE_DIR}/*.h)

  target_sources(libsqlite PRIVATE ${sqlite_SOURCE_DIR}/sqlite3.c)
  target_include_directories(libsqlite PUBLIC $<BUILD_INTERFACE:${sqlite_SOURCE_DIR}> $<INSTALL_INTERFACE:include/sqlite>)
  target_sources(libsqlite INTERFACE FILE_SET HEADERS BASE_DIRS ${sqlite_SOURCE_DIR} FILES ${head_files})

  if(UNIX)
    target_link_libraries(libsqlite PUBLIC pthread dl)
  endif()

  set_property(TARGET libsqlite PROPERTY EXPORT_NAME sqlite::libsqlite)
  install(
    TARGETS libsqlite
    EXPORT sqlite-config
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
            FILE_SET HEADERS
            DESTINATION include/sqlite)

  install(EXPORT sqlite-config DESTINATION lib/cmake/sqlite)
endif()

# import targets:
# sqlite::libsqlite
