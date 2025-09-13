#pragma once
#include "module.h"
#include "timer.h"
#include <sys/epoll.h>
#include <vector>
#include <memory>
#include <unistd.h>

// System类是应用程序的核心，管理epoll事件循环
class System {
public:
    System();
    ~System();
    
    // 删除拷贝构造和赋值操作
    System(const System&) = delete;
    System& operator=(const System&) = delete;
    

    
    // 初始化系统
    bool initialize();
    
    // 运行主事件循环
    void run();
    
    // 添加模块到系统
    void addModule(std::shared_ptr<Module> module);
    
    // 添加到epoll监控
    bool addToEpoll(int fd, std::shared_ptr<Module> module);
    
    // 从epoll中移除
    bool removeFromEpoll(int fd);
    
    // 获取模块管理器
    ModuleManager& getModuleManager();
    
    // 获取定时器
    Timer& getTimer();
    
    // 停止系统运行
    void stop();
    
private:

private:
    int epoll_fd_ = -1;
    ModuleManager module_manager_;
    Timer timer_;
    volatile bool running_ = false;
    
    // 创建epoll实例
    bool createEpoll();
    
    // 初始化所有模块
    void initializeModules();
    
    // 处理epoll事件
    void handleEvents(struct epoll_event* events, int nfds);
    
    // 输出i3bar协议头
    void outputProtocolHeader();
    
    // RAII包装器用于文件描述符
    class FdWrapper {
    public:
        explicit FdWrapper(int fd = -1) : fd_(fd) {}
        ~FdWrapper() { if (fd_ != -1) close(fd_); }
        
        FdWrapper(const FdWrapper&) = delete;
        FdWrapper& operator=(const FdWrapper&) = delete;
        
        FdWrapper(FdWrapper&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
        FdWrapper& operator=(FdWrapper&& other) noexcept {
            if (this != &other) {
                if (fd_ != -1) close(fd_);
                fd_ = other.fd_;
                other.fd_ = -1;
            }
            return *this;
        }
        
        int get() const { return fd_; }
        void reset(int fd = -1) {
            if (fd_ != -1) close(fd_);
            fd_ = fd;
        }
        
    private:
        int fd_;
    };
    
    FdWrapper epoll_fd_wrapper_;
};