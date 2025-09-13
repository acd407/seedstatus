#pragma once
#include "module.h"
#include "timer.h"
#include <sys/epoll.h>
#include <vector>
#include <memory>
#include <unistd.h>

/**
 * @file system.h
 * @brief 系统核心类的定义
 *
 * 本文件定义了System类，它是整个状态栏应用程序的核心。
 * System类负责管理epoll事件循环、模块生命周期、定时器等核心功能。
 *
 * 主要功能：
 * - epoll事件循环管理
 * - 模块的初始化、更新和销毁
 * - 定时器管理
 * - 信号处理集成
 * - i3bar协议输出
 *
 * 设计特点：
 * - 基于事件驱动的架构
 * - 高效的I/O多路复用
 * - 模块化设计，易于扩展
 * - 良好的错误处理和资源管理
 */

/**
 * @brief 系统核心类
 *
 * System类是整个状态栏应用程序的核心，负责管理所有模块和事件循环。
 * 它使用epoll来实现高效的事件驱动架构，支持文件描述符监控和定时器。
 *
 * 主要职责：
 * 1. 管理epoll实例和事件循环
 * 2. 初始化和管理所有模块
 * 3. 处理模块的文件描述符事件
 * 4. 管理定时器事件
 * 5. 输出符合i3bar协议的状态栏内容
 * 6. 处理系统信号，实现优雅关闭
 *
 * 使用示例：
 * @code
 * int main() {
 *     System system;
 *     if (!system.initialize()) {
 *         return EXIT_FAILURE;
 *     }
 *     system.run();
 *     return EXIT_SUCCESS;
 * }
 * @endcode
 */
class System {
  public:
    /**
     * @brief 构造函数
     *
     * 初始化System类的内部状态，但不创建epoll实例或启动事件循环。
     */
    System();

    /**
     * @brief 析构函数
     *
     * 自动清理所有资源，包括epoll实例和模块。
     */
    ~System();

    // 删除拷贝构造和赋值操作
    System(const System &) = delete;
    System &operator=(const System &) = delete;

    /**
     * @brief 初始化系统
     * @return true如果初始化成功，false如果失败
     *
     * 执行系统初始化操作，包括：
     * - 创建epoll实例
     * - 初始化所有模块
     * - 输出i3bar协议头
     * 必须在调用run()之前调用此方法。
     */
    bool initialize();

    /**
     * @brief 运行主事件循环
     *
     * 启动事件循环，等待并处理各种事件：
     * - 模块文件描述符的I/O事件
     * - 定时器事件
     * - 信号事件
     *
     * 此方法会阻塞，直到调用stop()方法或收到终止信号。
     */
    void run();

    /**
     * @brief 添加模块到系统
     * @param module 要添加的模块共享指针
     *
     * 将模块添加到系统中，并自动调用其init()方法。
     * 模块会被注册到模块管理器中，并可以参与事件循环。
     */
    void addModule(std::shared_ptr<Module> module);

    /**
     * @brief 添加文件描述符到epoll监控
     * @param fd 要监控的文件描述符
     * @param module 关联的模块
     * @return true如果添加成功，false如果失败
     *
     * 将文件描述符添加到epoll实例中进行监控。
     * 当fd有可读事件时，会自动调用关联模块的update()方法。
     */
    bool addToEpoll(int fd, std::shared_ptr<Module> module);

    /**
     * @brief 从epoll中移除文件描述符
     * @param fd 要移除的文件描述符
     * @return true如果移除成功，false如果失败
     *
     * 停止对指定文件描述符的监控。
     * 通常在模块被销毁或不再需要事件通知时调用。
     */
    bool removeFromEpoll(int fd);

    /**
     * @brief 获取模块管理器
     * @return 模块管理器的引用
     *
     * 提供对模块管理器的访问，用于高级操作。
     */
    ModuleManager &getModuleManager();

    /**
     * @brief 获取定时器
     * @return 定时器的引用
     *
     * 提供对定时器的访问，用于添加定时任务。
     */
    Timer &getTimer();

    /**
     * @brief 停止系统运行
     *
     * 请求停止事件循环，通常在收到终止信号时调用。
     * 系统会优雅地关闭，完成当前事件处理后退出。
     */
    void stop();

  private:
    int epoll_fd_ = -1;             ///< epoll文件描述符
    ModuleManager module_manager_;  ///< 模块管理器
    Timer timer_;                   ///< 定时器
    volatile bool running_ = false; ///< 运行状态标志

    /**
     * @brief 创建epoll实例
     * @return true如果创建成功，false如果失败
     *
     * 创建epoll实例并设置相关参数。
     */
    bool createEpoll();

    /**
     * @brief 初始化所有模块
     *
     * 调用所有模块的init()方法，执行模块特定的初始化操作。
     */
    void initializeModules();

    /**
     * @brief 处理epoll事件
     * @param events 事件数组
     * @param nfds 事件数量
     *
     * 处理epoll_wait返回的事件，调用相应模块的update()方法。
     */
    void handleEvents(struct epoll_event *events, int nfds);

    /**
     * @brief 输出i3bar协议头
     *
     * 输出i3bar协议要求的头部信息，包括版本和点击事件支持等。
     */
    void outputProtocolHeader();

    /**
     * @brief 文件描述符RAII包装器
     *
     * 用于自动管理文件描述符的生命周期，确保资源正确释放。
     * 遵循RAII（Resource Acquisition Is Initialization）原则。
     */
    class FdWrapper {
      public:
        /**
         * @brief 构造函数
         * @param fd 文件描述符，默认为-1
         */
        explicit FdWrapper(int fd = -1) : fd_(fd) {}

        /**
         * @brief 析构函数
         *
         * 自动关闭文件描述符（如果有效）。
         */
        ~FdWrapper() {
            if (fd_ != -1)
                close(fd_);
        }

        // 删除拷贝构造和赋值操作
        FdWrapper(const FdWrapper &) = delete;
        FdWrapper &operator=(const FdWrapper &) = delete;

        /**
         * @brief 移动构造函数
         * @param other 要移动的FdWrapper对象
         */
        FdWrapper(FdWrapper &&other) noexcept : fd_(other.fd_) {
            other.fd_ = -1;
        }

        /**
         * @brief 移动赋值操作符
         * @param other 要移动的FdWrapper对象
         * @return 当前对象的引用
         */
        FdWrapper &operator=(FdWrapper &&other) noexcept {
            if (this != &other) {
                if (fd_ != -1)
                    close(fd_);
                fd_ = other.fd_;
                other.fd_ = -1;
            }
            return *this;
        }

        /**
         * @brief 获取文件描述符
         * @return 文件描述符
         */
        int get() const {
            return fd_;
        }

        /**
         * @brief 重置文件描述符
         * @param fd 新的文件描述符，默认为-1
         *
         * 关闭当前文件描述符（如果有效）并设置新的文件描述符。
         */
        void reset(int fd = -1) {
            if (fd_ != -1)
                close(fd_);
            fd_ = fd;
        }

      private:
        int fd_; ///< 文件描述符
    };

    FdWrapper epoll_fd_wrapper_; ///< epoll文件描述符包装器
};