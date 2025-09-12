#include <modules/stdin.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>
#include <system.h>

// 定义缓冲区大小
#define BUF_SIZE 4096

StdinModule::StdinModule() : Module("stdin") {
    // Stdin模块不需要定时更新，只在有输入时更新
    setInterval(0);
}

StdinModule::~StdinModule() {}

void StdinModule::update() {
    // 解析输入内容
    parseInput();
}

void StdinModule::init() {

    // 设置标准输入为非阻塞模式

    setNonBlocking(STDIN_FILENO);

    // 检查文件描述符状态
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags != -1) {
    }

    // 设置文件描述符，这样系统会将其添加到epoll

    setFd(STDIN_FILENO);
}

void StdinModule::handleClick(uint64_t button) {
    // 这个模块本身不处理点击事件
    (void)button;
}

void StdinModule::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    }
}

void StdinModule::parseInput() {
    char input[BUF_SIZE];
    ssize_t n = read(STDIN_FILENO, input, BUF_SIZE - 1);

    if (n == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
        }
        return;
    }

    if (n == 0) {
        return;
    }

    // 确保输入以null结尾
    input[n] = '\0';

    // 尝试查找第一个JSON对象
    const char *json_start = strchr(input, '{');
    if (!json_start) {
        return;
    }

    try {
        // 尝试解析JSON对象
        auto json = nlohmann::json::parse(json_start, nullptr, false);

        if (json.is_discarded()) {
            return;
        }

        // 检查是否包含必要的字段
        if (json.contains("name") && json.contains("button")) {
            std::string module_name = json["name"];
            uint64_t button = json["button"];

            // 获取System实例
            System &system = System::getInstance();
            // 获取模块管理器
            ModuleManager &module_manager = system.getModuleManager();

            // 查找对应的模块
            auto module = module_manager.getModuleByName(module_name);

            if (module) {
                // 调用模块的handleClick方法
                module->handleClick(button);
            }
        }
    } catch (const std::exception &e) {
        // 处理所有可能的异常
    }
}
