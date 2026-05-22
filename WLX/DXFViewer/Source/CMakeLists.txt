cmake_minimum_required(VERSION 3.18)

# Use static MSVC runtime (/MT)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(wlx_dxf LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GENERATE OUTPUT .gitignore CONTENT "*")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- Dependencies via FetchContent ---
include(FetchContent)

# win-iconv (lightweight iconv for Windows)
FetchContent_Declare(
    win_iconv
    GIT_REPOSITORY https://github.com/win-iconv/win-iconv.git
    GIT_TAG 70a279dcfe6318bbe63e019e33b3230b55c19762
)
set(WIN_ICONV_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(WIN_ICONV_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(WIN_ICONV_BUILD_EXECUTABLE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(win_iconv)

# libdxfrw (DXF/DWG reading library)
FetchContent_Declare(
    libdxfrw
    GIT_REPOSITORY https://github.com/thbeu/libdxfrw.git
    GIT_TAG dd05b3fe1fe810b4072d63c26caba3a7eeac207d
)
FetchContent_GetProperties(libdxfrw)
if(NOT libdxfrw_POPULATED)
    FetchContent_Populate(libdxfrw)
endif()

# Build libdxfrw as a static library (custom target, not their CMakeLists.txt)
file(GLOB LIBDXFRW_SOURCES ${libdxfrw_SOURCE_DIR}/src/*.cpp)
file(GLOB LIBDXFRW_INTERN_SOURCES ${libdxfrw_SOURCE_DIR}/src/intern/*.cpp)
add_library(dxfrw STATIC ${LIBDXFRW_SOURCES} ${LIBDXFRW_INTERN_SOURCES})
target_include_directories(dxfrw PUBLIC ${libdxfrw_SOURCE_DIR}/src)
target_include_directories(dxfrw PRIVATE ${win_iconv_SOURCE_DIR})
target_link_libraries(dxfrw PRIVATE iconv_static)
target_compile_definitions(dxfrw PRIVATE _CRT_SECURE_NO_WARNINGS ICONV_CONST=)
if(MSVC)
    target_compile_options(dxfrw PRIVATE /W3 /wd4244 /wd4267 /wd4996)
endif()

# --- Plugin DLL ---
add_library(wlx_dxf SHARED
    src/dllmain.cpp
    src/wlx_dxf.cpp
    src/wlx_dxf.rc
    src/listplug.h
    src/resource.h
    src/dxf_collector.h
    src/dxf_renderer.h
)

target_include_directories(wlx_dxf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${libdxfrw_SOURCE_DIR}/src
)

target_link_libraries(wlx_dxf PRIVATE dxfrw gdiplus ole32)

target_compile_definitions(wlx_dxf PRIVATE
    WIN32 _WINDOWS NOMINMAX _WIN32_WINNT=0x0502 UNICODE _UNICODE
)

target_compile_options(wlx_dxf PRIVATE "/utf-8")

# Module-definition file for controlled exports
set_target_properties(wlx_dxf PROPERTIES
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/src/wlx_dxf.def\""
)

# Output name and extension: dxfViewer.wlx (32-bit) or dxfViewer.wlx64 (64-bit)
set_target_properties(wlx_dxf PROPERTIES OUTPUT_NAME "dxfViewer")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(wlx_dxf PROPERTIES SUFFIX ".wlx64")
else()
    set_target_properties(wlx_dxf PROPERTIES SUFFIX ".wlx")
endif()

# Copy binary to package/ after Release build
add_custom_command(TARGET wlx_dxf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E $<IF:$<CONFIG:Release>,copy_if_different,true>
        "$<TARGET_FILE:wlx_dxf>"
        "${CMAKE_CURRENT_SOURCE_DIR}/package/$<TARGET_FILE_NAME:wlx_dxf>"
)

# --- Stand-alone test exe ---
add_executable(test_dxf src/test_dxf.cpp)
target_compile_definitions(test_dxf PRIVATE WIN32 _WINDOWS UNICODE _UNICODE)
target_compile_options(test_dxf PRIVATE "/utf-8")

# --- dwg2dxf converter ---
add_executable(dwg2dxf
    ${libdxfrw_SOURCE_DIR}/dwg2dxf/main.cpp
    ${libdxfrw_SOURCE_DIR}/dwg2dxf/dx_iface.cpp
    ${libdxfrw_SOURCE_DIR}/dwg2dxf/dx_data.h
    ${libdxfrw_SOURCE_DIR}/dwg2dxf/dx_iface.h
)
target_include_directories(dwg2dxf PRIVATE
    ${libdxfrw_SOURCE_DIR}/src
    ${libdxfrw_SOURCE_DIR}/dwg2dxf
)
target_link_libraries(dwg2dxf PRIVATE dxfrw)
target_compile_definitions(dwg2dxf PRIVATE _CRT_SECURE_NO_WARNINGS)
if(MSVC)
    target_compile_options(dwg2dxf PRIVATE /W3 /wd4244 /wd4267 /wd4996)
endif()
