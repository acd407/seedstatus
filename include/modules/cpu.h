#pragma once
#include "module.h"
#include <cstdint>

// CPU模块 - 显示CPU使用率和功率消耗
class CpuModule : public Module {
  public:
    CpuModule();
    ~CpuModule();

    // 更新模块状态
    virtual void update() override;

    // 处理点击事件
    virtual void handleClick(uint64_t button) override;

  private:
    // 获取CPU使用率
    double getUsage();

    // 获取CPU功率消耗
    double getPower();

    // 私有数据
    uint64_t prev_idle_ = 0;             // 上一次的空闲时间
    uint64_t prev_total_ = 0;            // 上一次的总时间
    uint64_t prev_energy_ = 0;           // 上一次的能量消耗
    uint64_t rapl_max_energy_range_ = 0; // RAPL 最大能量数值，超过就溢出了

    // 定义文件路径常量
    static constexpr const char *PACKAGE = "/sys/class/powercap/intel-rapl:0/energy_uj";
    static constexpr const char *CORE = "/sys/class/powercap/intel-rapl:0:0/energy_uj";
    static constexpr const char *RAPL_MAX_ENERGY_RANGE =
        "/sys/class/powercap/intel-rapl:0:0/max_energy_range_uj";
    static constexpr const char *SVI2_P_Core = "/sys/class/hwmon/hwmon3/power1_input";
    static constexpr const char *SVI2_P_SoC = "/sys/class/hwmon/hwmon3/power2_input";
    static constexpr const char *PROC_STAT = "/proc/stat";

    // 是否使用RAPL接口获取功率
    static constexpr bool USE_RAPL = true;
};
