#pragma once
#include "module.h"
#include <cstdint>

// Memory模块 - 显示内存使用情况
class MemoryModule : public Module {
  public:
    MemoryModule();
    ~MemoryModule();

    // 更新模块状态
    virtual void update() override;

    // 处理点击事件
    virtual void handleClick(uint64_t button) override;

  private:
    // 获取内存使用情况
    void getUsage(uint64_t &used, double &percent);

    // 格式化存储单位
    std::string formatStorageUnits(double bytes);

    // 私有数据
    uint64_t prev_used_ = 0;    // 上一次的已用内存
    double prev_percent_ = 0.0; // 上一次的使用百分比

    // 定义文件路径常量
    static constexpr const char *MEMINFO = "/proc/meminfo";
};