#pragma once
#include "module.h"
#include <vector>
#include <sys/timerfd.h>
#include <memory>
#include <unistd.h>

// Timer类负责管理时间更新的模块
class Timer {
  public:
    Timer();
    ~Timer();

    // 删除拷贝构造和赋值操作
    Timer(const Timer &) = delete;
    Timer &operator=(const Timer &) = delete;

    // 允许移动操作
    Timer(Timer &&) = default;
    Timer &operator=(Timer &&) = default;

    // 初始化定时器
    bool initialize(int epoll_fd);

    // 获取定时器文件描述符
    int getFd() const;

    // 添加需要按时间间隔更新的模块
    void addIntervalModule(std::shared_ptr<Module> module);

    // 移除模块
    void removeModule(const std::shared_ptr<Module> &module);

    // 处理定时器事件，更新需要更新的模块
    void handleTimerEvent();

    // 更新定时器
    void update();

    // 检查某个模块是否需要在当前计数器值更新
    bool shouldUpdate(uint64_t counter, std::shared_ptr<Module> module);

    // 获取当前计数器值
    uint64_t getCounter() const;

    // 设置定时器间隔
    bool setInterval(uint64_t seconds);

  private:
    int timer_fd_ = -1;
    volatile uint64_t counter_ = 0;
    int epoll_fd_ = -1;
    std::vector<std::shared_ptr<Module>> interval_modules_;

    // 创建定时器文件描述符
    int createTimerFd();

    // 读取定时器事件
    uint64_t readTimerFd();

    // RAII包装器用于文件描述符
    class FdWrapper {
      public:
        explicit FdWrapper(int fd = -1) : fd_(fd) {}
        ~FdWrapper() {
            if (fd_ != -1)
                close(fd_);
        }

        FdWrapper(const FdWrapper &) = delete;
        FdWrapper &operator=(const FdWrapper &) = delete;

        FdWrapper(FdWrapper &&other) noexcept : fd_(other.fd_) {
            other.fd_ = -1;
        }
        FdWrapper &operator=(FdWrapper &&other) noexcept {
            if (this != &other) {
                if (fd_ != -1)
                    close(fd_);
                fd_ = other.fd_;
                other.fd_ = -1;
            }
            return *this;
        }

        int get() const {
            return fd_;
        }
        void reset(int fd = -1) {
            if (fd_ != -1)
                close(fd_);
            fd_ = fd;
        }

      private:
        int fd_;
    };

    FdWrapper timer_fd_wrapper_;
};