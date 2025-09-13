#include <modules/stdin.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>
#include <system.h>

// 定义缓冲区大小
#define BUF_SIZE 4096

StdinModule::StdinModule(System *system) : Module("stdin"), system_(system) {
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
        std::cerr << "StdinModule: Successfully got stdin flags: " << flags << std::endl;
    } else {
        std::cerr << "StdinModule: Failed to get stdin flags: " << strerror(errno) << std::endl;
    }

    // 设置文件描述符，这样系统会将其添加到epoll

    setFd(STDIN_FILENO);
    std::cerr << "StdinModule: Registered fd " << STDIN_FILENO << " for epoll" << std::endl;
}

void StdinModule::handleClick(uint64_t button) {
    // 这个模块本身不处理点击事件
    (void)button;
}

void StdinModule::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "StdinModule: Failed to get file descriptor flags for fd " << fd << ": "
                  << strerror(errno) << std::endl;
        return;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "StdinModule: Failed to set non-blocking mode for fd " << fd << ": "
                  << strerror(errno) << std::endl;
    } else {
        std::cerr << "StdinModule: Successfully set non-blocking mode for fd " << fd << std::endl;
    }
}

void StdinModule::parseInput() {
    char input[BUF_SIZE];
    ssize_t n = read(STDIN_FILENO, input, BUF_SIZE - 1);

    if (n == -1) {
        // 检查是否为非阻塞IO的正常返回
        if (errno != EAGAIN) {
#if EWOULDBLOCK != EAGAIN
            if (errno != EWOULDBLOCK) {
#endif
                // 真正的错误处理
                std::cerr << "StdinModule: Error reading from stdin: " << strerror(errno)
                          << std::endl;
#if EWOULDBLOCK != EAGAIN
            } else {
                std::cerr << "StdinModule: stdin would block (EWOULDBLOCK)" << std::endl;
            }
#else
            std::cerr << "StdinModule: stdin would block (EAGAIN)" << std::endl;
#endif
        }
        return;
    }

    if (n == 0) {
        std::cerr << "StdinModule: EOF received on stdin" << std::endl;
        return;
    }

    // 确保输入以null结尾
    input[n] = '\0';

    // 尝试查找第一个JSON对象
    const char *json_start = strchr(input, '{');
    if (!json_start) {
        std::cerr << "StdinModule: No JSON object found in input" << std::endl;
        return;
    }

    try {
        // 尝试解析JSON对象
        auto json = nlohmann::json::parse(json_start, nullptr, false);

        if (json.is_discarded()) {
            std::cerr << "StdinModule: Failed to parse JSON input" << std::endl;
            return;
        }

        // 检查是否包含必要的字段
        if (json.contains("name") && json.contains("button")) {
            std::string module_name = json["name"];
            uint64_t button = json["button"];

            // 使用System实例指针
            if (!system_) {
                std::cerr << "StdinModule: System pointer is null, cannot handle click event"
                          << std::endl;
                return;
            }
            System &system = *system_;
            // 获取模块管理器
            ModuleManager &module_manager = system.getModuleManager();

            // 查找对应的模块
            auto module = module_manager.getModuleByName(module_name);

            if (module) {
                // 调用模块的handleClick方法
                std::cerr << "StdinModule: Forwarding click event to module " << module_name
                          << " with button " << button << std::endl;
                module->handleClick(button);
            } else {
                std::cerr << "StdinModule: Module " << module_name << " not found" << std::endl;
            }
        }
    } catch (const std::exception &e) {
        // 处理所有可能的异常
        std::cerr << "StdinModule: Exception while parsing input: " << e.what() << std::endl;
    }
}