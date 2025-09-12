#pragma once
#include "module.h"
#include <string>
#include <vector>

// 背光模块 - 控制屏幕背光亮度
class BacklightModule : public Module {
public:
    BacklightModule();
    ~BacklightModule();
    
    // 删除拷贝构造和赋值操作
    BacklightModule(const BacklightModule&) = delete;
    BacklightModule& operator=(const BacklightModule&) = delete;
    
    // 允许移动操作
    BacklightModule(BacklightModule&&) = default;
    BacklightModule& operator=(BacklightModule&&) = default;
    
    // 更新模块状态
    virtual void update() override;
    
    // 处理点击事件
    virtual void handleClick(uint64_t button) override;
    
    // 初始化模块
    virtual void init() override;
    
private:
    // 获取背光亮度百分比
    uint64_t getBrightnessPercent();
    
    // 获取背光图标
    std::string getBrightnessIcon(uint64_t brightness_percent);
    
    // 格式化输出字符串
    std::string formatOutput(uint64_t brightness_percent);
    
    // 背光亮度文件路径
    static const std::string BRIGHTNESS_PATH;
    
    // 最大背光亮度文件路径
    static const std::string MAX_BRIGHTNESS_PATH;
    
    // inotify文件描述符
    int inotify_fd_ = -1;
    
    // 监视描述符
    int watch_fd_ = -1;
    
    // 背光图标定义
    static const std::vector<std::string> brightness_icons_;
};