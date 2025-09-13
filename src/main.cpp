#include <system.h>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <memory>

namespace {
volatile bool g_running = true;
System *g_system = nullptr;

// 信号处理函数
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..."
              << std::endl;
    g_running = false;
    if (g_system) {
        g_system->stop();
    }
}

// 设置信号处理
void setupSignalHandlers() {
    struct sigaction sa;
    sa.sa_handler = signalHandler;
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

int main() {
    try {
        // 设置信号处理
        setupSignalHandlers();

        // 直接创建System实例
        System system;
        g_system = &system;

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