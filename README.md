# SeedStatus

<span style="white-space: nowrap;">
[![License](https://img.shields.io/github/license/acd407/seedstatus)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.20+-green.svg)](https://cmake.org/)
[![Linux](https://img.shields.io/badge/Linux-compatible-brightgreen.svg)](https://www.linux.org/)
</span>

## 项目概述

SeedStatus 是一个用现代 C++20 编写的模块化状态栏程序，专为 i3 窗口管理器和 Sway 合成器设计。它采用事件驱动架构，提供高性能、低资源占用的系统状态监控和显示功能。

### 主要特点

- **模块化设计**：每个功能模块独立开发，易于扩展和维护
- **事件驱动架构**：基于 epoll 的高效 I/O 多路复用
- **现代 C++**：使用 C++20 特性，代码简洁高效
- **低资源占用**：优化的内存使用和 CPU 性能
- **实时更新**：支持实时状态更新和事件响应
- **i3bar 协议兼容**：完全兼容 i3bar 和 swaybar 协议

## 功能特性

### 核心功能

- **电池状态监控**：实时显示电池电量、充电状态、剩余时间
- **音频控制**：显示音量级别，支持静音切换
- **背光调节**：监控和显示屏幕背光亮度
- **标准输入处理**：处理 i3/sway 的点击事件

### 技术特性

- **异步事件处理**：基于 epoll 的事件循环
- **定时器支持**：内置定时器系统，支持周期性任务
- **信号处理**：优雅的信号处理，支持安全关闭
- **错误处理**：完善的异常处理和错误恢复机制
- **调试支持**：丰富的调试信息和日志输出

## 系统要求

### 最低要求

- **编译器**：支持 C++20 的 GCC/Clang
- **CMake**：3.20 或更高版本

### 依赖项

- **nlohmann_json**：JSON 解析库
- **sdbus-c++**：D-Bus C++ 绑定
- **ALSA**：音频系统库
- **systemd**：系统服务管理（可选）

## 安装说明

### 从源码构建

#### 1. 克隆仓库

```bash
git clone https://github.com/acd407/seedstatus.git
cd seedstatus
```

#### 2. 安装依赖项

**Ubuntu/Debian:**

```bash
sudo apt update
sudo apt install build-essential cmake libnlohmann-json-dev libsdbus-c++-dev libasound2-dev
```

**Fedora/CentOS:**

```bash
sudo dnf install cmake nlohmann-json-devel sdbus-c++-devel alsa-lib-devel
```

**Arch Linux:**

```bash
sudo pacman -Syu cmake nlohmann-json sdbus-cpp alsa-lib
```

#### 3. 构建项目

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make -j$(nproc)
```

#### 4. 安装

```bash
# 安装到系统
sudo make install
```

### 构建选项

```bash
# Debug 构建（包含调试信息）
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release 构建（优化版本）
cmake -DCMAKE_BUILD_TYPE=Release ..

# 自定义安装路径
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

## 使用方法

### 基本使用

```bash
# 运行状态栏
seedstatus

# 在 i3 配置中使用
bar {
    status_command seedstatus
}
```

### 模块配置

每个模块都可以独立配置，具体配置方法请参考相应模块的文档。

## 项目架构

### 整体架构

```
┌─────────────────────────────────────────────────────┐
│                        SeedStatus                   │
├─────────────────────────────────────────────────────┤
│                    System Core                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │   Event     │  │   Module    │  │   Timer     │  │
│  │   Loop      │  │  Manager    │  │   System    │  │
│  └─────────────┘  └─────────────┘  └─────────────┘  │
├─────────────────────────────────────────────────────┤
│                      Modules                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │   Battery   │  │    Audio    │  │  Backlight  │  │
│  │   Module    │  │   Module    │  │   Module    │  │
│  └─────────────┘  └─────────────┘  └─────────────┘  │
│  ┌─────────────┐                                    │
│  │    Stdin    │                                    │
│  │   Module    │                                    │
│  └─────────────┘                                    │
├─────────────────────────────────────────────────────┤
│                    Dependencies                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │   epoll     │  │    D-Bus    │  │    ALSA     │  │
│  │   System    │  │   System    │  │   System    │  │
│  └─────────────┘  └─────────────┘  └─────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 核心组件

#### System 类

System 类是整个应用程序的核心，负责：

- 管理 epoll 事件循环
- 初始化和管理所有模块
- 处理定时器事件
- 输出 i3bar 协议格式

#### Module 基类

Module 是所有功能模块的基类，定义了统一的接口：

- `init()`: 模块初始化
- `update()`: 状态更新
- `handleClick()`: 处理点击事件
- `getOutput()`: 获取输出内容

#### ModuleManager

ModuleManager 负责管理所有模块的生命周期：

- 模块注册和注销
- 模块状态管理
- 模块间通信

### 模块系统

#### BatteryModule

电池状态监控模块，提供：

- 电池电量百分比
- 充电/放电状态
- 剩余时间估算
- 低电量警告

**依赖项：** D-Bus, UPower

#### AudioModule

音频控制模块，提供：

- 主音量显示
- 静音状态指示
- 音量调节支持
- 多设备支持

**依赖项：** ALSA

#### BacklightModule

背光控制模块，提供：

- 背光亮度百分比
- 亮度调节支持
- 自动亮度适配

**依赖项：** sysfs, inotify

#### StdinModule

标准输入处理模块，提供：

- i3bar 点击事件处理
- JSON 消息解析
- 模块间事件分发

**依赖项：** nlohmann_json

### 创建新模块

#### 1. 创建模块头文件

```cpp
// include/modules/mymodule.h
#pragma once
#include "../module.h"

class MyModule : public Module {
public:
    MyModule();
    ~MyModule();

    bool init() override;
    void update() override;
    void handleClick(const std::string& action) override;
    std::string getOutput() const override;

private:
    // 模块特定的成员变量
    int some_value_;
};
```

#### 2. 实现模块功能

```cpp
// src/modules/mymodule.cpp
#include "modules/mymodule.h"
#include <iostream>

MyModule::MyModule() : some_value_(0) {
    // 构造函数实现
}

MyModule::~MyModule() {
    // 析构函数实现
}

bool MyModule::init() {
    // 初始化代码
    return true;
}

void MyModule::update() {
    // 更新状态
}

void MyModule::handleClick(const std::string& action) {
    // 处理点击事件
}

std::string MyModule::getOutput() const {
    // 返回输出内容
    return "My Module: " + std::to_string(some_value_);
}
```

#### 3. 注册模块

```cpp
// src/main.cpp
#include "modules/mymodule.h"

int main() {
    System system;
    system.addModule(std::make_shared<MyModule>());
    // ... 其他代码
}
```

### 日志分析

程序输出包含丰富的调试信息：

```bash
# 运行并查看详细日志
./seedstatus 2>&1 | tee seedstatus.log

# 过滤错误信息
grep -i error seedstatus.log

# 过滤警告信息
grep -i warning seedstatus.log
```

---

**SeedStatus** - 为 i3/sway 用户提供简洁、高效的状态栏解决方案
