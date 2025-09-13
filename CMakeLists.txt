cmake_minimum_required(VERSION 3.14)

# 项目信息
project(seedstatus
    VERSION 1.0.0
    DESCRIPTION "System status monitoring tool"
    LANGUAGES CXX
)

# C++标准设置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 启用编译命令导出（用于IDE和代码分析工具）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 启用ccache加速编译（如果可用）
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()


# 基础警告选项 - 对项目代码启用基本警告检查
set(PROJECT_BASE_WARNINGS
    -Wall              # 启用所有基本警告
    -Wextra            # 启用额外的有用警告
    -Wpedantic         # 严格遵循C++标准
    -Werror            # 将警告视为错误
)

# 严格警告选项 - 对项目代码启用深度代码质量检查
set(PROJECT_STRICT_WARNINGS
    # 内存和指针安全
    -Wshadow           # 警告变量阴影
    -Wnull-dereference # 警告可能的空指针解引用
    
    # 类型转换安全
    -Wcast-align       # 警告可能导致对齐问题的指针转换
    -Wcast-qual        # 警告去除类型限定符的转换
    
    # 逻辑和控制流
    -Wduplicated-branches # 警告重复的条件分支
    -Wduplicated-cond  # 警告重复的条件
    -Wlogical-op       # 警告逻辑操作符中的可疑用法
    -Wswitch-default   # 警告switch语句缺少default
    -Wswitch-enum      # 警告switch语句未处理所有枚举值
    
    # 格式化和字符串安全
    -Wformat=2          # 严格的格式字符串检查
    -Wformat-security   # 警告格式字符串安全漏洞
    -Wformat-nonliteral # 警告非字面量格式字符串
    
    # 变量和初始化
    -Wunused-parameter    # 警告未使用的参数
    -Wunused-variable     # 警告未使用的变量
    -Wunused-but-set-variable # 警告被设置但未使用的变量
    -Wuninitialized       # 警告未初始化的变量
    -Winit-self           # 警告自初始化
    -Wmissing-field-initializers # 警告缺失的字段初始化器
    
    # C++特定警告
    -Wmissing-declarations # 警告全局函数缺少声明
    -Wnoexcept             # 警告可能抛出异常的函数
    -Wstrict-null-sentinel # 警告空指针哨兵用法
    -Woverloaded-virtual   # 警告隐藏的虚函数
    -Wsign-promo           # 警告可能导致符号问题的隐式转换
    -Wzero-as-null-pointer-constant # 警告使用0作为空指针常量
    
    # 包含文件
    -Wmissing-include-dirs # 警告缺失的包含目录
)

# 可选警告选项 - 可能会产生较多警告，但有助于发现潜在问题
set(PROJECT_OPTIONAL_WARNINGS
    -Wconversion        # 警告隐式类型转换
    -Wsign-conversion   # 警告可能导致符号改变的转换
    -Wold-style-cast    # 警告C风格类型转换
    -Wuseless-cast      # 警告无用的类型转换
    -Wdouble-promotion  # 警告浮点数双精度提升
    -Wtrampolines       # 警告trampoline生成
    -Wfloat-equal       # 警告浮点数相等比较
    -Wunsafe-loop-optimizations # 警告可能不安全的循环优化
)

# 系统头文件警告抑制 - 避免第三方库警告干扰
set(SYSTEM_WARNING_SUPPRESSION
    -w  # 完全禁用系统头文件警告
)


# 设置默认构建类型（如果没有指定）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Debug模式配置
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring Debug build")
    add_compile_definitions(DEBUG=1)
    
    # Debug编译选项
    add_compile_options(-g)
    add_compile_options(-O0) # 禁用优化以便调试
    
    # 内存安全检查（Sanitizers）
    add_compile_options(
        -fsanitize=address      # 地址消毒器
        -fsanitize=undefined    # 未定义行为消毒器
        -fsanitize=leak         # 内存泄漏消毒器
        -fno-omit-frame-pointer # 保留栈帧以便更好的错误报告
    )
    
    message(STATUS "Debug build with sanitizers enabled")

# Release模式配置
else()
    message(STATUS "Configuring Release build")
    add_compile_options(-O2)  # 优化级别2
    add_compile_options(-DNDEBUG)  # 禁用调试代码
    
    # 可选的链接时优化（需要编译器支持）
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORTED)
    if(IPO_SUPPORTED)
        message(STATUS "IPO/LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
endif()


# 应用系统头文件警告抑制（全局生效）
add_compile_options(${SYSTEM_WARNING_SUPPRESSION})

# 查找现代CMake包
find_package(nlohmann_json REQUIRED)
find_package(sdbus-c++ REQUIRED)

# 查找系统包（使用pkg-config）
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)
pkg_check_modules(DBUS REQUIRED dbus-1)

# 收集项目源文件
file(GLOB_RECURSE PROJECT_SOURCES 
    "src/*.cpp"
    "src/*.h"
    "src/*.hpp"
)

# 创建可执行文件目标
add_executable(seedstatus ${PROJECT_SOURCES})

# 设置目标属性
set_target_properties(seedstatus PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "seedstatus"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 配置包含目录
target_include_directories(seedstatus 
    # 项目头文件（严格警告检查）
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    
    # 第三方库头文件（抑制警告）
    SYSTEM PUBLIC 
        ${ALSA_INCLUDE_DIRS}
        ${DBUS_INCLUDE_DIRS}
)

# 应用项目特定的编译警告选项
target_compile_options(seedstatus PRIVATE 
    ${PROJECT_BASE_WARNINGS}
    ${PROJECT_STRICT_WARNINGS}
    ${PROJECT_OPTIONAL_WARNINGS}
)

# 链接依赖库
target_link_libraries(seedstatus PRIVATE 
    nlohmann_json::nlohmann_json
    ${ALSA_LIBRARIES}
    ${DBUS_LIBRARIES}
)

# Debug模式下的额外链接选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(seedstatus PRIVATE
        -fsanitize=address
        -fsanitize=undefined
        -fsanitize=leak
        -fno-omit-frame-pointer
    )
endif()

# 安装目标
install(TARGETS seedstatus DESTINATION bin)
