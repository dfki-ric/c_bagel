project(c_bagel)
set(PROJECT_VERSION 1.0)
set(PROJECT_DESCRIPTION "C implementation to load, save, modify and execute Bagel graphs")
cmake_minimum_required(VERSION 2.6)
include(FindPkgConfig)

find_package(lib_manager)
define_module_info()

option(YAML_SUPPORT "Add support for loading graphs from YAML files." ON)
option(INTERVAL_SUPPORT "Add support for Interval Arithmetic." OFF)
option(UNIT_TESTS "Compile Unittests." OFF)
option(DOUBLE_PRECISION "Compile Unittests." ON)

include_directories(
  src
  ${CMAKE_BINARY_DIR}/src
)

if(DOUBLE_PRECISION)
  set(BG_PRECISION double)
else(DOUBLE_PRECISION)
  set(BG_PRECISION float)
endif(DOUBLE_PRECISION)

if(YAML_SUPPORT)
  pkg_check_modules(YAML REQUIRED yaml-0.1)
  include_directories(${YAML_INCLUDE_DIRS})
  link_directories(${YAML_LIBRARY_DIRS})
  add_definitions(-DYAML_SUPPORT ${YAML_CFLAGS_OTHER})
  set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} yaml)
if(APPLE)
  include_directories(/opt/local/include)
endif(APPLE)
endif(YAML_SUPPORT)

if(INTERVAL_SUPPORT)
  # MPFI doesn't ship a package-config file :(
  #pkg_check_modules(MPFI REQUIRED mpfi)
  #include_directories(${MPFI_INCLUDE_DIRS})
  #link_directories(${MPFI_LIBRARY_DIRS})
  add_definitions(-DINTERVAL_SUPPORT)
  set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} mpfi)
if(APPLE)
  include_directories(/opt/local/include)
  set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} mpfr gmp)
endif(APPLE)
endif(INTERVAL_SUPPORT)

if(UNIX)
  set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} dl)
endif(UNIX)

set(SOURCES
  ${SOURCES}
  src/tsort/tsort.c
  src/bg.c
  src/bg_graph.c
  src/bg_node.c
  src/bg_edge.c
  src/bg_interval.c
  src/generic_list.c
  src/node_list.c
  src/edge_list.c
  src/bg_yaml_loader.c
  src/bg_yaml_writer.c
  src/node_types/bg_node_atomic.c
  src/node_types/bg_node_subgraph.c
  src/node_types/bg_node_extern.c
  src/node_types/bg_node_ports.c
  src/merge_types/bg_merge_basic.c
)

set(HEADERS
  ${HEADERS}
  src/bagel.h
  src/bg_impl.h
  src/bg_node.h
)

configure_file(src/config.h.in ${CMAKE_BINARY_DIR}/src/config.h @ONLY)

if(UNIT_TESTS)
  enable_testing()
  add_executable(test ${SOURCES} src/test_bagel.c)
  # TODO: ERROR we should only overwrite the cmake_flags for test target
  #if(TEST_COVERAGE)
    # overwrite compiler flags to enable test coverage and disable optimizations
    #set(CMAKE_C_FLAGS "-Wall -Werror -fprofile-arcs -ftest-coverage")
  #endif(TEST_COVERAGE)
  add_subdirectory(tests)

  target_link_libraries(test
    ${PKGCONFIG_LIBRARIES}
    ${EXTRA_LIBRARIES}
    m
    )
endif(UNIT_TESTS)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -Wextra -Wshadow -std=c99 -pedantic")

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_link_libraries(${PROJECT_NAME}
                      ${PKGCONFIG_LIBRARIES}
                      ${EXTRA_LIBRARIES}
                      m
)

if(WIN32)
  set(LIB_INSTALL_DIR bin) # .dll are in PATH, like executables
else(WIN32)
  set(LIB_INSTALL_DIR lib)
endif(WIN32)


set(_INSTALL_DESTINATIONS
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION lib
)


# Install the library into the lib folder
install(TARGETS ${PROJECT_NAME} ${_INSTALL_DESTINATIONS})

# Install headers into mars include directory
install(FILES ${CMAKE_BINARY_DIR}/src/config.h DESTINATION include/${PROJECT_NAME})
install(FILES ${HEADERS} DESTINATION include/${PROJECT_NAME})

# Prepare and install necessary files to support finding of the library
# using pkg-config
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
INSTALL(PROGRAMS python/generate_extern_node.py DESTINATION bin)
INSTALL(PROGRAMS python/bg_yaml_to_dot.py DESTINATION bin)
