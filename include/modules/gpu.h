#pragma once
#include "module.h"
#include <cstdint>

// GPU模块 - 显示显卡使用率和显存占用
class GpuModule : public Module {
public:
    GpuModule();
    ~GpuModule();
    
    // 更新模块状态
    virtual void update() override;
    
    // 处理点击事件
    virtual void handleClick(uint64_t button) override;
    
private:
    // 获取GPU使用率
    uint64_t getGpuUsage();
    
    // 获取显存使用量
    uint64_t getVramUsed();
    
    // 格式化存储单位
    void formatStorageUnits(char* buffer, uint64_t bytes);
    
    // 私有数据
    bool show_vram_ = false;      // 是否显示显存占用
    
    // 定义文件路径常量
    static constexpr const char* GPU_USAGE = "/sys/class/drm/card1/device/gpu_busy_percent";
    static constexpr const char* VRAM_USED = "/sys/class/drm/card1/device/mem_info_vram_used";
};