cmake_minimum_required(VERSION 3.16)
project(audio_link_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE COMMON_SOURCES src/common/*.cpp)
file(GLOB_RECURSE CLI_SOURCES cli/*.cpp)
file(GLOB_RECURSE FFI_SOURCES src/ffi/*.cpp)

# On Apple/Windows, AudioDeviceManager is implemented in platform-specific sources.
# Exclude the generic (GStreamer-based) implementation to avoid duplicate symbols.
if(APPLE OR WIN32)
    list(FILTER COMMON_SOURCES EXCLUDE REGEX ".*src/common/audio_device_manager\\.cpp$")
endif()

add_library(${PROJECT_NAME} SHARED ${COMMON_SOURCES} ${FFI_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# macOS
if(APPLE)
    file(GLOB_RECURSE MACOS_SOURCES src/macos/*.cpp)

    target_sources(${PROJECT_NAME} PRIVATE ${MACOS_SOURCES})
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            "-framework CoreAudio"
            "-framework CoreFoundation"
        )
    target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include/macos)
endif()

# Windows
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    target_compile_definitions(${PROJECT_NAME} PRIVATE AUDIO_LINK_CORE_DLL_EXPORTS)

    file(GLOB_RECURSE WINDOWS_SOURCES src/windows/*.cpp)

    target_sources(${PROJECT_NAME} PRIVATE ${WINDOWS_SOURCES})
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
    target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include/windows)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include/common)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include/ffi)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/external)

if(ANDROID)
    target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE  
        "${GSTREAMER_ROOT_ANDROID}/include"
        "${GSTREAMER_ROOT_ANDROID}/include/gstreamer-1.0"
        "${GSTREAMER_ROOT_ANDROID}/include/glib-2.0"
        "${GSTREAMER_ROOT_ANDROID}/lib/glib-2.0/include"
    )
    target_link_directories(${PROJECT_NAME} PRIVATE "${GSTREAMER_ROOT_ANDROID}/lib")
    target_link_libraries(${PROJECT_NAME} PRIVATE gstreamer-1.0 gstapp-1.0 gmodule-2.0 gobject-2.0 glib-2.0 pcre2-8 ffi intl iconv)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
    target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${GSTREAMER_INCLUDE_DIRS})
    target_link_directories(${PROJECT_NAME} PRIVATE ${GSTREAMER_LIBRARY_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GSTREAMER_LIBRARIES})

    # Build CLI executable
    add_executable(${PROJECT_NAME}_cli ${CLI_SOURCES})
    target_link_libraries(${PROJECT_NAME}_cli PRIVATE ${PROJECT_NAME})
endif()
