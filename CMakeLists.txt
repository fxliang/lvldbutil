cmake_minimum_required(VERSION 3.20)
project(lvldbutil)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

# options
option(USE_BUNDLED_CXXOPTS "Use built-in version of cxxopts" ${WIN32})
option(USE_BUNDLED_LEVELDB "Use built-in version of leveldb" ${WIN32})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
# Force MSVC to use static runtime libraries
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(MINGW)
  set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
endif()

# dependencies
if (USE_BUNDLED_CXXOPTS)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/cxxopts)
else()
  find_package(cxxopts)
  if (NOT cxxopts_FOUND)
    message(WARNING "pkg-config could not find cxxopts; falling back to vendored deps/cxxopts/")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/cxxopts)
  endif()
endif()

if (USE_BUNDLED_LEVELDB)
  set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "Build LevelDB's unit tests")
  set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "Build LevelDB's benchmarks")
  set(LEVELDB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/leveldb/include)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/leveldb)
else()
  find_package(LevelDb REQUIRED)
  if (NOT LevelDb_FOUND)
    message(WARNING "pkg-config could not find leveldb; falling back to vendored deps/leveldb/")
    set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "Build LevelDB's unit tests")
    set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "Build LevelDB's benchmarks")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/leveldb)
  endif()
endif()
set(LEVELDB_LIBRARIES leveldb)

aux_source_directory(./src DIR_SRC)
set(lvldbutil ${DIR_SRC})
add_executable(${PROJECT_NAME} ${lvldbutil})

if (WIN32)
  target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
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
