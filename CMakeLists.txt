cmake_minimum_required(VERSION 3.27.7)

# ==========================================
# 1. 项目基础配置
# ==========================================
project(frame_window_counter)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================
# 2. Geode SDK 初始化
# ==========================================
if(NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
endif()
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# ==========================================
# 3. 目标与源文件定义
# ==========================================
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.hpp"
)

add_library(${PROJECT_NAME} SHARED ${SOURCES} "src/Audio/SoundManager.cpp" )

setup_geode_mod(${PROJECT_NAME})

# ==========================================
# 4. 第三方依赖项管理
# ==========================================
# --- 依赖 A: Silicate (slc) 宏格式库 ---
include(FetchContent)
FetchContent_Declare(
    slc_repo
    GIT_REPOSITORY https://git.puppy.lgbt/silicate/slc.git
    GIT_TAG        main 
)
# 仅拉取源码以获取头文件，跳过内部编译链，避免 MSVC 语法报错
FetchContent_GetProperties(slc_repo)
if(NOT slc_repo_POPULATED)
    FetchContent_Populate(slc_repo)
endif()
target_include_directories(${PROJECT_NAME} PRIVATE ${slc_repo_SOURCE_DIR}/include)

# --- 依赖 B: GDR 格式及其转换器 ---
CPMAddPackage("gh:maxnut/GDReplayFormat#2a0ca3c")
CPMAddPackage(
    NAME GDRconverter
    GITHUB_REPOSITORY maxnut/GDR-converter
    GIT_TAG main
    EXCLUDE_FROM_ALL YES
    OPTIONS "BUILD_TESTING OFF"
)

# ==========================================
# 5. 链接库
# ==========================================
target_link_libraries(${PROJECT_NAME} libGDR GDRconverter)