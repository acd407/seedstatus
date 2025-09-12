cmake_minimum_required(VERSION 3.14)
project(seedstatus CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 生成compile_commands.json文件，用于IDE和代码分析工具
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 添加编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building in Debug mode")
    add_compile_options(-Wall -Wextra -pedantic -g)
    add_compile_definitions(DEBUG=1)
else()
    message(STATUS "Building in Release mode")
    add_compile_options(-Wall -Wextra -pedantic -O2)
endif()

# 查找依赖
find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)
pkg_check_modules(DBUS REQUIRED dbus-1)

# 添加头文件搜索路径
include_directories(include)

# 添加源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(seedstatus ${SOURCES})

# 链接库
target_link_libraries(seedstatus PRIVATE nlohmann_json::nlohmann_json ${ALSA_LIBRARIES} ${DBUS_LIBRARIES})
target_include_directories(seedstatus PRIVATE ${ALSA_INCLUDE_DIRS} ${DBUS_INCLUDE_DIRS})

# 安装规则
install(TARGETS seedstatus DESTINATION bin)