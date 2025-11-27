cmake_minimum_required(VERSION 3.16)
project(Audio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE CLI_SOURCES cli/*.cpp)

add_library(audio_lib SHARED ${LIB_SOURCES})
set_target_properties(audio_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)

# macOS
if(APPLE)
    enable_language(OBJCXX)
    enable_language(Swift)

    file(GLOB_RECURSE MACOS_SOURCES 
        macos/*.mm
        macos/*.cpp
        macos/*.swift
    )

    target_sources(audio_lib PRIVATE ${MACOS_SOURCES})
    set_target_properties(audio_lib PROPERTIES Swift_LANGUAGE_VERSION "5")
    target_link_libraries(audio_lib
        PRIVATE
            "-framework CoreAudio"
            "-framework CoreFoundation"
        )
    target_include_directories(audio_lib PRIVATE ${PROJECT_SOURCE_DIR}/macos)
endif()

# Windows
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

    file(GLOB_RECURSE WINDOWS_SOURCES 
        windows/*.cpp
    )

    target_sources(audio_lib PRIVATE ${WINDOWS_SOURCES})
    target_link_libraries(audio_lib PRIVATE ws2_32)
    target_include_directories(audio_lib PRIVATE ${PROJECT_SOURCE_DIR}/windows)
endif()

target_include_directories(audio_lib PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_include_directories(audio_lib SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/external)

if(ANDROID)
    target_include_directories(audio_lib SYSTEM PRIVATE  
        "${GSTREAMER_ROOT_ANDROID}/include"
        "${GSTREAMER_ROOT_ANDROID}/include/gstreamer-1.0"
        "${GSTREAMER_ROOT_ANDROID}/include/glib-2.0"
        "${GSTREAMER_ROOT_ANDROID}/lib/glib-2.0/include"
    )
    target_link_directories(audio_lib PRIVATE "${GSTREAMER_ROOT_ANDROID}/lib")
    target_link_libraries(audio_lib PRIVATE gstreamer-1.0 gmodule-2.0 gobject-2.0 glib-2.0 pcre2-8 ffi intl iconv)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
    
    target_include_directories(audio_lib SYSTEM PRIVATE ${GSTREAMER_INCLUDE_DIRS})
    target_link_directories(audio_lib PRIVATE ${GSTREAMER_LIBRARY_DIRS})
    target_link_libraries(audio_lib PRIVATE ${GSTREAMER_LIBRARIES})
    message(STATUS "GStreamer include dirs: ${GSTREAMER_LIBRARIES}")
endif()

# Build CLI executable
if(NOT ANDROID)
    add_executable(audio_cli ${CLI_SOURCES})
    target_link_libraries(audio_cli PRIVATE audio_lib)
endif()