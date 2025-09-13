#include "modules/backlight.h"
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

// 静态成员定义
const std::string BacklightModule::BRIGHTNESS_PATH = "/sys/class/backlight/amdgpu_bl1/brightness";
const std::string BacklightModule::MAX_BRIGHTNESS_PATH =
    "/sys/class/backlight/amdgpu_bl1/max_brightness";

const std::vector<std::string> BacklightModule::brightness_icons_ = {"", "", "", "",
                                                                     "", "", "", "",
                                                                     "", "", "", "",
                                                                     "", "", ""};

BacklightModule::BacklightModule() : Module("backlight") {
    // 背光模块默认不基于时间间隔更新，而是基于inotify事件
    setInterval(0);
}

BacklightModule::~BacklightModule() {
    // 清理inotify资源
    if (watch_fd_ != -1) {
        inotify_rm_watch(inotify_fd_, watch_fd_);
    }
    if (inotify_fd_ != -1) {
        close(inotify_fd_);
    }
}

void BacklightModule::init() {
    // 初始化inotify
    inotify_fd_ = inotify_init1(IN_NONBLOCK);
    if (inotify_fd_ == -1) {
        std::cerr << "Failed to initialize inotify: " << strerror(errno) << std::endl;
        setOutput("󰛨", Color::DEACTIVE);
        setInterval(1); // 设置重试间隔
        return;
    }

    // 添加需要监控的文件
    watch_fd_ = inotify_add_watch(inotify_fd_, BRIGHTNESS_PATH.c_str(), IN_MODIFY);
    if (watch_fd_ == -1) {
        std::cerr << "Failed to add watch for " << BRIGHTNESS_PATH << ": " << strerror(errno)
                  << std::endl;
        close(inotify_fd_);
        inotify_fd_ = -1;
        setOutput("󰛨", Color::DEACTIVE);
        setInterval(1); // 设置重试间隔
        return;
    }

    // 设置文件描述符，System会将其添加到epoll
    setFd(inotify_fd_);
    std::cerr << "BacklightModule registered fd " << inotify_fd_ << " for epoll" << std::endl;

    // 立即更新一次
    update();
}

void BacklightModule::update() {
    try {
        // 读取inotify事件
        if (inotify_fd_ != -1) {
            char buffer[1024];
            ssize_t len = read(inotify_fd_, buffer, sizeof(buffer));
            if (len == -1 && errno != EAGAIN) {
                std::cerr << "Error reading inotify events: " << strerror(errno) << std::endl;
                return;
            }
        }

        // 获取背光亮度百分比
        uint64_t brightness_percent = getBrightnessPercent();

        // 格式化输出
        std::string output = formatOutput(brightness_percent);

        // 设置输出
        setOutput(output, Color::IDLE);

    } catch (const std::exception &e) {
        std::cerr << "BacklightModule update error: " << e.what() << std::endl;
        setOutput("󰛨", Color::DEACTIVE);
        setInterval(1); // 设置重试间隔
    }
}

void BacklightModule::handleClick(uint64_t button) {
    switch (button) {
    case 4: // 上滚 - 增加背光
        std::cerr << "Backlight: Increasing brightness" << std::endl;
        system("~/.bin/wm/backlight i >/dev/null &");
        break;
    case 5: // 下滚 - 减少背光
        std::cerr << "Backlight: Decreasing brightness" << std::endl;
        system("~/.bin/wm/backlight d >/dev/null &");
        break;
    default:
        break;
    }
}

uint64_t BacklightModule::getBrightnessPercent() {
    // 读取当前亮度值
    uint64_t brightness = readUint64File(BRIGHTNESS_PATH);
    uint64_t max_brightness = readUint64File(MAX_BRIGHTNESS_PATH);

    if (max_brightness == 0) {
        return 0;
    }

    // 计算百分比
    uint64_t brightness_percent = brightness * 100 / max_brightness;
    if (brightness_percent > 100) {
        brightness_percent = 100;
    }

    // 按照5的倍数进行舍入
    brightness_percent = (brightness_percent + 1) / 5 * 5;

    return brightness_percent;
}

std::string BacklightModule::getBrightnessIcon(uint64_t brightness_percent) {
    if (brightness_icons_.empty()) {
        return "󰛨";
    }

    // 根据亮度百分比选择图标
    size_t idx = brightness_icons_.size() * brightness_percent / 101;
    if (idx >= brightness_icons_.size()) {
        idx = brightness_icons_.size() - 1;
    }

    return brightness_icons_[idx];
}

std::string BacklightModule::formatOutput(uint64_t brightness_percent) {
    std::ostringstream output;
    output << getBrightnessIcon(brightness_percent) << "\u2004";

    // 格式化百分比显示
    if (brightness_percent == 100) {
        output << std::setw(3) << brightness_percent << "%";
    } else {
        output << std::setw(2) << brightness_percent << "%";
    }

    return output.str();
}