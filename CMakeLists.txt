
cmake_minimum_required( VERSION 3.1 )
project( recovery_gui )


set( CMAKE_C_STANDARD 11 )
set( CMAKE_C_STANDARD_REQUIRED TRUE )

file( GLOB SOURCES "*.c" )
file( GLOB HEADERS "*.h" )

set( SOURCE_FILES
     ${SOURCES}
     ${HEADERS}
     )

add_executable( ${PROJECT_NAME} ${SOURCE_FILES} )

set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )

find_library( LIB_LVGL NAMES lvgl )

target_compile_options( ${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Werror
    -Os
    )

target_link_libraries( ${PROJECT_NAME} PRIVATE
    swupdate
    Threads::Threads
    ${LIB_LVGL}
    rt
    )

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SYSROOT}/usr/include/lvgl
    )

install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
    )

if( BUILD_TESTING )
    enable_testing()
    add_subdirectory( unittests )
endif()
