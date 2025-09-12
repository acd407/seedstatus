#pragma once
#include "module.h"
#include <dbus/dbus.h>
#include <string>
#include <vector>

class BatteryModule : public Module {
public:
    BatteryModule();
    ~BatteryModule() override;
    
    // 重写基类方法
    void init() override;
    void update() override;
    void handleClick(uint64_t button) override;
    
private:
    // 电池状态枚举
    enum class BatteryState {
        UNKNOWN,
        CHARGING,
        DISCHARGING,
        EMPTY,
        FULLY_CHARGED,
        PENDING_CHARGE,
        PENDING_DISCHARGE
    };
    
    // DBus相关方法
    void dbusGetProperty(const std::string& device_path, const std::string& property_name, 
                         int expected_type, void* value);
    void getBatteryInfo(BatteryState& state, double& percentage, int64_t& time, 
                        double& energy, double& energy_rate);
    
    // 辅助方法
    std::string getBatteryIcon(BatteryState state, uint64_t percentage);
    std::string formatTime(int64_t seconds);
    std::string formatOutput(BatteryState state, uint64_t percentage, 
                            double energy, double energy_rate, int64_t time);
    
    // 静态常量
    static const std::string UPOWER_SERVICE;
    static const std::string DEVICE_INTERFACE;
    static const std::string BATTERY_PATH;
    
    // 充电图标数组
    static const std::vector<std::string> charging_icons_;
    
    // 放电图标数组
    static const std::vector<std::string> discharging_icons_;
    
    // DBus连接
    DBusConnection* dbus_conn_;
    
    // 显示模式：true显示详细模式（能量信息），false显示简单模式（百分比）
    bool detailed_mode_;
};

// 静态成员定义
inline const std::string BatteryModule::UPOWER_SERVICE = "org.freedesktop.UPower";
inline const std::string BatteryModule::DEVICE_INTERFACE = "org.freedesktop.UPower.Device";
inline const std::string BatteryModule::BATTERY_PATH = "/org/freedesktop/UPower/devices/battery_BAT0";

inline const std::vector<std::string> BatteryModule::charging_icons_ = {
    "󰂆", "󰂇", "󰂈", "󰂉", "󰂊", "󰂋", "󰂅"
};

inline const std::vector<std::string> BatteryModule::discharging_icons_ = {
    "󰂎", "󰁺", "󰁻", "󰁼", "󰁽", "󰁾", "󰁿", "󰂀", "󰂁", "󰂂", "󰁹"
};