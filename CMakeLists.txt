cmake_minimum_required(VERSION 3.16)
project(Audio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES
    cli/*.cpp
    src/*.cpp
)
add_executable(Audio ${SOURCES})

if(APPLE)
    enable_language(OBJCXX)
    enable_language(Swift)

    file(GLOB_RECURSE MACOS_SOURCES 
        src/macos/*.mm
        src/macos/*.cpp
        src/macos/*.swift
    )

    target_sources(Audio PRIVATE ${MACOS_SOURCES})
    set_target_properties(Audio PROPERTIES Swift_LANGUAGE_VERSION "5")

    target_link_libraries(Audio
        PRIVATE
            "-framework CoreAudio"
            "-framework CoreFoundation"
        )
endif()

if(WIN32)
    target_link_libraries(Audio PRIVATE ws2_32)
endif()

target_include_directories(Audio PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(Audio SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/external)

# GStreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

target_include_directories(Audio SYSTEM PRIVATE ${GSTREAMER_INCLUDE_DIRS})
target_link_directories(Audio PRIVATE ${GSTREAMER_LIBRARY_DIRS})
target_link_libraries(Audio PRIVATE ${GSTREAMER_LIBRARIES})