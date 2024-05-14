
cmake_minimum_required( VERSION 3.1 )
project( unittests )


# gtest require pthread to be used
set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads )

find_package( PkgConfig REQUIRED )

pkg_check_modules( LIB_GTEST REQUIRED IMPORTED_TARGET gtest_main gmock)

find_library( LIB_LVGL NAMES lvgl )


file( GLOB REC_TEST_SOURCE_FILES "*.cpp" )

add_executable( ${PROJECT_NAME}
    ${REC_TEST_SOURCE_FILES}
    ../rec-util-files.c
    ../rec-util-files.h
    ../rec-util-config.c
    ../rec-util-config.h
    ../rec-util-networking.c
    ../rec-util-networking.h
    ../rec-util-system.c
    ../rec-util-system.h
    ../rec-util-linked-list.c
    ../rec-util-linked-list.h
    ../rec-lvgl-custom-tick.c
    )

target_include_directories( ${PROJECT_NAME} PUBLIC
    ..
    )

target_compile_options( ${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Werror
    -Os
    )

target_link_libraries( ${PROJECT_NAME} PRIVATE
    PkgConfig::LIB_GTEST
    Threads::Threads
    ${LIB_LVGL}
    )

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SYSROOT}/usr/include/lvgl
    )

target_compile_features( ${PROJECT_NAME} PRIVATE
    cxx_std_17
    )

add_test( NAME UnitTest
          COMMAND qemu-aarch64 -L ${CMAKE_SYSROOT} unittests --gtest_output=xml:results.xml
          )

