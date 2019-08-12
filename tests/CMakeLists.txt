project(test_c_bagel)
set(PROJECT_VERSION 1.0)
set(PROJECT_DESCRIPTION "Unit Tests for the c_bagel Library")
cmake_minimum_required(VERSION 2.6)
include(FindPkgConfig)

find_package(lib_manager)
#lib_defaults()
enable_testing()

pkg_check_modules(TEST_PKGCONFIG REQUIRED
  c_bagel
  check
)
include_directories(${TEST_PKGCONFIG_INCLUDE_DIRS})
link_directories(${TEST_PKGCONFIG_LIBRARY_DIRS})
add_definitions(${TEST_PKGCONFIG_CFLAGS_OTHER})  #flags excluding the ones with -I

include_directories(. ../src)

set(SOURCES test_all.c test_api.c)

if(YAML_SUPPORT)
  set(SOURCES ${SOURCES} test_yaml.c)
  if(INTERVAL_SUPPORT)
    set(SOURCES ${SOURCES} test_interval.c)
  endif(INTERVAL_SUPPORT)
endif(YAML_SUPPORT)


set(HEADERS ${HEADERS})

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${TEST_PKGCONFIG_LIBRARIES} m pthread)

add_test(${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_graphs DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
