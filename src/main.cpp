/**
 * @file main.cpp
 * @brief 程序入口点和主循环
 *
 * 本文件包含程序的主入口点，负责：
 * - 设置信号处理
 * - 初始化系统
 * - 运行主事件循环
 * - 处理异常和错误
 *
 * 程序流程：
 * 1. 设置信号处理（SIGINT, SIGTERM）
 * 2. 创建System实例
 * 3. 初始化系统
 * 4. 运行主事件循环
 * 5. 处理异常并退出
 */

#include <system.h>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <functional>

namespace {
/**
 * @brief 全局信号处理函数
 *
 * 使用std::function存储信号处理函数，允许使用lambda捕获局部变量。
 */
static std::function<void(int)> g_signalHandler;

/**
 * @brief 信号处理函数包装器
 * @param signal 收到的信号编号
 *
 * 这是一个静态函数，作为信号处理程序的入口点。
 * 它调用存储在g_signalHandler中的实际处理函数。
 */
static void signalHandlerWrapper(int signal) {
    if (g_signalHandler) {
        g_signalHandler(signal);
    }
}

/**
 * @brief 设置信号处理
 * @param handler 信号处理函数
 *
 * 为SIGINT和SIGTERM信号设置处理函数。
 * 这些信号通常由用户发送（Ctrl+C）或系统发送（终止进程）。
 *
 * @throws std::runtime_error 如果设置信号处理失败
 */
static void setupSignalHandlers(const std::function<void(int)> &handler) {
    g_signalHandler = handler;

    struct sigaction sa;
    sa.sa_handler = signalHandlerWrapper;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        throw std::runtime_error("Failed to set SIGINT handler");
    }

    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        throw std::runtime_error("Failed to set SIGTERM handler");
    }
}
} // namespace

/**
 * @brief 程序主入口点
 * @return 程序退出代码（0表示成功，非0表示失败）
 *
 * 程序的主入口点，负责初始化系统并运行主事件循环。
 *
 * 主要步骤：
 * 1. 创建System实例
 * 2. 设置信号处理
 * 3. 初始化系统
 * 4. 运行主事件循环
 * 5. 处理异常并退出
 *
 * 异常处理：
 * - 捕获std::exception及其子类
 * - 捕获未知异常
 * - 输出错误信息到stderr
 *
 * 返回值：
 * - EXIT_SUCCESS (0): 程序成功执行
 * - EXIT_FAILURE (1): 程序执行失败
 */
int main() {
    try {
        // 直接创建System实例
        System system;

        // 设置信号处理，使用lambda捕获system对象
        setupSignalHandlers([&system](int signal) {
            const char *signalName;
            switch (signal) {
            case SIGINT:
                signalName = "SIGINT";
                break;
            case SIGTERM:
                signalName = "SIGTERM";
                break;
            default:
                signalName = "UNKNOWN";
                break;
            }

            std::cerr << "\nReceived signal " << signalName << " (" << signal
                      << "), shutting down..." << std::endl;
            system.stop();
        });

        // 初始化系统
        if (!system.initialize()) {
            std::cerr << "Failed to initialize system" << std::endl;
            return EXIT_FAILURE;
        }

        // 运行主循环
        system.run();

    } catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}