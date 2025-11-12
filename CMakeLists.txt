cmake_minimum_required(VERSION 3.20)
project(lvldbutil)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

# options
option(USE_BUNDLED_CXXOPTS "Use built-in version of cxxopts" ${WIN32})
option(USE_BUNDLED_LEVELDB "Use built-in version of leveldb" ${WIN32})

# Force MSVC to use static runtime libraries
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# dependencies
if (USE_BUNDLED_CXXOPTS)
  add_subdirectory(deps/cxxopts)
  set(CXXOPTS_INCLUDE_DIR deps/cxxopts/include)
else()
  find_package(PkgConfig REQUIRED)  # Add this line
  find_package(cxxopts)
  if (cxxopts_FOUND)
    set(CXXOPTS_INCLUDE_DIR ${CXXOPTS_INCLUDE_DIRS})
  else()
    message(WARNING "pkg-config could not find cxxopts; falling back to vendored cxxopts/")
    add_subdirectory(deps/cxxopts)
    set(CXXOPTS_INCLUDE_DIR deps/cxxopts/include)
  endif()
endif()

if (USE_BUNDLED_LEVELDB)
  set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "Build LevelDB's unit tests")
  set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "Build LevelDB's benchmarks")
  set(LEVELDB_LIBRARIES leveldb)
  set(LEVELDB_INCLUDE_DIR deps/leveldb/include)
  add_subdirectory(deps/leveldb)
else()
  find_package(PkgConfig REQUIRED)  # Add this line
  pkg_search_module(LEVELDB leveldb)
  if (LEVELDB_FOUND)
    set(LEVELDB_INCLUDE_DIR ${LEVELDB_INCLUDE_DIRS})
  else()
    message(WARNING "pkg-config could not find leveldb; falling back to vendored leveldb/")
    set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "Build LevelDB's unit tests")
    set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "Build LevelDB's benchmarks")
    add_subdirectory(deps/leveldb)
    set(LEVELDB_LIBRARIES leveldb)
    set(LEVELDB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/leveldb/include)
  endif()
endif()

aux_source_directory(./src DIR_SRC)
set(lvldbutil ${DIR_SRC})
add_executable(${PROJECT_NAME} ${lvldbutil})

if (WIN32)
  set(WIN32_LIBS shlwapi user32 Rpcrt4)
endif()

if(MSVC)
  target_link_options(${PROJECT_NAME} PRIVATE "/LTCG")
endif()

# add link libraries
target_link_libraries(${PROJECT_NAME}
  cxxopts::cxxopts
  ${LEVELDB_LIBRARIES}
  ${WIN32_LIBS}
)

# add include directories
target_include_directories(${PROJECT_NAME} PUBLIC
  ${LEVELDB_INCLUDE_DIR}
  ${CXXOPTS_INCLUDE_DIR}
)

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
