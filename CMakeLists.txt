cmake_minimum_required(VERSION 3.20)
project(lvldbutil)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

# options
option(USE_BUNDLED_CXXOPTS "Use built-in version of cxxopts" ${WIN32})
option(USE_BUNDLED_LEVELDB "Use built-in version of leveldb" ${WIN32})

# dependencies
if (USE_BUNDLED_CXXOPTS)
  add_subdirectory(deps/cxxopts)
  set(CXXOPTS_INCLUDE_DIR deps/cxxopts/include)
else()
  find_package(cxxopts REQUIRED)
endif()

if (USE_BUNDLED_LEVELDB)
  set(LEVELDB_BUILD_TESTS OFF)
  set(LEVELDB_BUILD_BENCHMARKS OFF)
  set(LEVELDB_LIBRARIES leveldb)
  set(LEVELDB_INCLUDE_DIR deps/leveldb/include)
  add_subdirectory(deps/leveldb)
else()
  find_package(PkgConfig REQUIRED)  # Add this line
  find_package(leveldb REQUIRED)
  pkg_search_module(LEVELDB REQUIRED leveldb)
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
