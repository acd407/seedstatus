#include <system.h>
#include <modules/date.h>
#include <modules/temp.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <mutex>

// 初始化静态成员
System *System::instance_ = nullptr;
std::mutex System::instance_mutex_;
std::once_flag System::init_flag_;

System::System() = default;

System::~System() = default;

// 线程安全的单例实现
System& System::getInstance() {
    std::call_once(init_flag_, [] {
        static System instance;
        instance_ = &instance;
    });
    return *instance_;
}

bool System::initialize() {
    try {
        // 创建epoll实例
        if (!createEpoll()) {
            return false;
        }

        // 初始化定时器
        if (!timer_.initialize(epoll_fd_wrapper_.get())) {
            epoll_fd_wrapper_.reset();
            return false;
        }

        // 将定时器添加到epoll
        if (!addToEpoll(timer_.getFd(), nullptr)) {
            epoll_fd_wrapper_.reset();
            return false;
        }

        // 初始化所有模块
        initializeModules();

        // 输出i3bar协议头
        outputProtocolHeader();

        running_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "System initialization failed: " << e.what() << std::endl;
        epoll_fd_wrapper_.reset();
        return false;
    } catch (...) {
        std::cerr << "System initialization failed with unknown error" << std::endl;
        epoll_fd_wrapper_.reset();
        return false;
    }
}

void System::run() {
    constexpr int MAX_EVENTS = 16;
    struct epoll_event events[MAX_EVENTS];

    // 初始输出所有模块
    module_manager_.outputModules();

    // 主事件循环
    while (running_) {
        int nfds = epoll_wait(epoll_fd_wrapper_.get(), events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) {
                continue; // 被信号中断，继续循环
            }
            std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
            continue;
        }

        // 处理所有事件
        try {
            handleEvents(events, nfds);
        } catch (const std::exception& e) {
            std::cerr << "Error handling events: " << e.what() << std::endl;
        }

        // 输出所有模块的更新
        module_manager_.outputModules();
    }
}

void System::stop() {
    running_ = false;
}

void System::addModule(std::shared_ptr<Module> module) {
    if (!module) {
        throw std::invalid_argument("Module cannot be null");
    }

    try {
        module_manager_.addModule(module);

        // 初始化模块
        module->init();

        // 如果模块有文件描述符，则添加到epoll
        int fd = module->getFd();
        if (fd != -1) {
            if (!addToEpoll(fd, module)) {
                throw std::runtime_error("Failed to add module to epoll");
            }
        }

        // 如果模块有更新间隔，则添加到定时器
        if (module->getInterval() > 0) {
            timer_.addIntervalModule(module);
        }

        // 立即更新一次
        module->update();
    } catch (const std::exception& e) {
        std::cerr << "Failed to add module " << module->getName() << ": " << e.what() << std::endl;
        throw;
    }
}

bool System::addToEpoll(int fd, std::shared_ptr<Module> module) {
    if (fd < 0) {
        return false;
    }
    
    struct epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;

    // 使用void*存储模块指针
    ev.data.ptr = module.get();

    if (epoll_ctl(epoll_fd_wrapper_.get(), EPOLL_CTL_ADD, fd, &ev) == -1) {
        std::cerr << "Failed to add fd " << fd << " to epoll: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

bool System::removeFromEpoll(int fd) {
    if (fd < 0) {
        return false;
    }
    
    if (epoll_ctl(epoll_fd_wrapper_.get(), EPOLL_CTL_DEL, fd, nullptr) == -1) {
        std::cerr << "Failed to remove fd " << fd << " from epoll: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

ModuleManager &System::getModuleManager() {
    return module_manager_;
}

Timer &System::getTimer() {
    return timer_;
}

bool System::createEpoll() {
    int fd = epoll_create1(EPOLL_CLOEXEC);
    if (fd == -1) {
        std::cerr << "Failed to create epoll instance: " << strerror(errno) << std::endl;
        return false;
    }
    
    epoll_fd_wrapper_.reset(fd);
    return true;
}

#include <modules/stdin.h>
#include <modules/cpu.h>
#include <modules/memory.h>
#include <modules/gpu.h>
#include <modules/network.h>
#include <modules/audio.h>
#include <modules/backlight.h>
#include <modules/battery.h>

void System::initializeModules() {
    // 添加Stdin模块，用于处理点击事件
    addModule(std::make_shared<StdinModule>());
    // 按照指定顺序初始化模块
    addModule(std::make_shared<BatteryModule>()); // Battery Status
    addModule(std::make_shared<BacklightModule>()); // Backlight Control
    addModule(std::make_shared<MicrophoneModule>()); // Microphone Control
    addModule(std::make_shared<VolumeModule>()); // Volume Control
    addModule(std::make_shared<NetworkModule>()); // Network Status
    addModule(std::make_shared<GpuModule>()); // GPU Usage
    addModule(std::make_shared<MemoryModule>()); // Memory Usage
    auto p = std::make_shared<CpuModule>(); // CPU Power
    p->setState(1);
    addModule(p);
    addModule(std::make_shared<CpuModule>()); // CPU Usage
    addModule(std::make_shared<TempModule>());
    addModule(std::make_shared<DateModule>());
}

void System::handleEvents(struct epoll_event *events, int nfds) {

    for (int i = 0; i < nfds; ++i) {

        // 检查是否是定时器事件
        if (events[i].data.ptr == nullptr) {
            timer_.update();
        } else {
            // 其他模块事件
            Module *module = static_cast<Module *>(events[i].data.ptr);
            if (module) {
                module->update();
            }
        }
    }
}

void System::outputProtocolHeader() {
    std::cout << "{ \"version\": 1, \"click_events\": true }" << std::endl;
    std::cout << '[' << std::endl;
    std::cout << "[]," << std::endl;
    std::cout.flush();
}