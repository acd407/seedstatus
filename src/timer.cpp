#include <timer.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <algorithm>

Timer::Timer() = default;

Timer::~Timer() = default;

bool Timer::initialize(int epoll_fd) {
    try {
        epoll_fd_ = epoll_fd;

        // 创建定时器文件描述符
        int fd = createTimerFd();
        if (fd == -1) {
            return false;
        }

        timer_fd_wrapper_.reset(fd);

        // 设置定时器
        struct itimerspec new_value{};
        new_value.it_value.tv_sec = 1;
        new_value.it_value.tv_nsec = 0;
        new_value.it_interval.tv_sec = 1;
        new_value.it_interval.tv_nsec = 0;

        if (timerfd_settime(timer_fd_wrapper_.get(), 0, &new_value, nullptr) == -1) {
            std::cerr << "Failed to set timer: " << strerror(errno) << std::endl;
            timer_fd_wrapper_.reset();
            return false;
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Timer initialization failed: " << e.what() << std::endl;
        timer_fd_wrapper_.reset();
        return false;
    } catch (...) {
        std::cerr << "Timer initialization failed with unknown error" << std::endl;
        timer_fd_wrapper_.reset();
        return false;
    }
}

int Timer::getFd() const {
    return timer_fd_wrapper_.get();
}

void Timer::addIntervalModule(std::shared_ptr<Module> module) {
    if (!module) {
        throw std::invalid_argument("Module cannot be null");
    }

    interval_modules_.push_back(module);
}

void Timer::removeModule(const std::shared_ptr<Module> &module) {
    if (!module) {
        return;
    }

    auto it = std::remove(interval_modules_.begin(), interval_modules_.end(), module);
    interval_modules_.erase(it, interval_modules_.end());
}

void Timer::handleTimerEvent() {
    try {
        // 读取定时器事件
        uint64_t expirations = readTimerFd();
        if (expirations > 0) {
            counter_ += expirations;
            const uint64_t current_counter = counter_;

            // 更新所有需要按时间间隔更新的模块
            for (const auto &module : interval_modules_) {
                if (!module)
                    continue;

                try {
                    if (module->getInterval() > 0 && current_counter % module->getInterval() == 0) {
                        module->update();
                    }
                } catch (const std::exception &e) {
                    std::cerr << "Error updating module " << module->getName() << ": " << e.what()
                              << std::endl;
                }
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error in handleTimerEvent: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error in handleTimerEvent" << std::endl;
    }
}

void Timer::update() {
    handleTimerEvent();
}

bool Timer::shouldUpdate(uint64_t counter, std::shared_ptr<Module> module) {
    return module && module->getInterval() > 0 && counter % module->getInterval() == 0;
}

uint64_t Timer::getCounter() const {
    return counter_;
}

bool Timer::setInterval(uint64_t seconds) {
    if (seconds == 0) {
        return false;
    }

    struct itimerspec new_value{};
    new_value.it_value.tv_sec = static_cast<time_t>(seconds);
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = static_cast<time_t>(seconds);
    new_value.it_interval.tv_nsec = 0;

    if (timerfd_settime(timer_fd_wrapper_.get(), 0, &new_value, nullptr) == -1) {
        std::cerr << "Failed to set timer interval: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

int Timer::createTimerFd() {
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timer_fd == -1) {
        std::cerr << "Failed to create timer fd: " << strerror(errno) << std::endl;
        return -1;
    }
    return timer_fd;
}

uint64_t Timer::readTimerFd() {
    uint64_t expirations = 0;
    ssize_t s = read(timer_fd_wrapper_.get(), &expirations, sizeof(uint64_t));

    if (s == -1) {
        if (errno != EAGAIN) {
            std::cerr << "Failed to read timer fd: " << strerror(errno) << std::endl;
        }
        return 0;
    }

    if (s != sizeof(uint64_t)) {
        std::cerr << "Unexpected read size from timer fd: " << s << std::endl;
        return 0;
    }

    return expirations;
}