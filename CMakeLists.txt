cmake_minimum_required(VERSION 3.14)
project(seedstatus CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 生成compile_commands.json文件，用于IDE和代码分析工具
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 严格编译选项
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wunused
    -Wuninitialized
    -Wnull-dereference
    -Wold-style-cast
    -Wcast-align
    -Wcast-qual
    -Wduplicated-branches
    -Wduplicated-cond
    -Wlogical-op
    -Wuseless-cast
)

# 添加编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building in Debug mode")
    add_compile_definitions(DEBUG=1)
    add_compile_options(-g)
    add_compile_options(
        -fsanitize=address
        -fsanitize=undefined
        -fsanitize=leak
        -fno-omit-frame-pointer
    )
    target_link_libraries(seedstatus PRIVATE
        -fsanitize=address
        -fsanitize=undefined
        -fsanitize=leak
        -fno-omit-frame-pointer
    )
else()
    message(STATUS "Building in Release mode")
    add_compile_options(-O2)
endif()

# 查找依赖
find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)
pkg_check_modules(DBUS REQUIRED dbus-1)

# 添加头文件搜索路径
include_directories(SYSTEM include)

# 添加源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(seedstatus ${SOURCES})

# 链接库
target_link_libraries(seedstatus PRIVATE nlohmann_json::nlohmann_json ${ALSA_LIBRARIES} ${DBUS_LIBRARIES})
target_include_directories(seedstatus PRIVATE ${ALSA_INCLUDE_DIRS} ${DBUS_INCLUDE_DIRS})

# 安装规则
install(TARGETS seedstatus DESTINATION bin)
