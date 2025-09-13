#include <system.h>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <functional>

namespace {
// 全局信号处理函数
static std::function<void(int)> g_signalHandler;

// 信号处理函数包装器
static void signalHandlerWrapper(int signal) {
    if (g_signalHandler) {
        g_signalHandler(signal);
    }
}

// 设置信号处理
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
