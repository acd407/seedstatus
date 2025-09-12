#pragma once
#include "module.h"
#include <cstdint>
#include <string>

// 网络模块 - 显示网络状态和流量
class NetworkModule : public Module {
public:
    NetworkModule();
    ~NetworkModule();
    
    // 更新模块状态
    virtual void update() override;
    
    // 处理点击事件
    virtual void handleClick(uint64_t button) override;
    
private:
    // 获取无线网络状态
    void getWirelessStatus(const std::string& ifname, int64_t& link, int64_t& level);
    
    // 获取网络速度和主设备
    void getNetworkSpeedAndMasterDev(uint64_t& rx, uint64_t& tx, std::string& master);
    
    // 格式化以太网输出
    void formatEtherOutput(std::ostringstream& output, const std::string& ifname, uint64_t rx, uint64_t tx);
    
    // 格式化无线网络输出
    void formatWirelessOutput(std::ostringstream& output, const std::string& ifname, uint64_t rx, uint64_t tx);
    
    // 格式化存储单位
    void formatStorageUnits(std::ostringstream& output, uint64_t bytes);
    
    // 私有数据
    bool show_details_ = false;    // 是否显示详细信息
    uint64_t prev_rx_ = 0;         // 上一次接收字节数
    uint64_t prev_tx_ = 0;         // 上一次发送字节数
    
    // 定义文件路径常量
    static constexpr const char* WIRELESS_STATUS = "/proc/net/wireless";
    static constexpr const char* NET_DEV = "/proc/net/dev";
    static constexpr const char* CARRIER_PATH_TEMPLATE = "/sys/class/net/%s/carrier";
    static constexpr const char* SPEED_PATH_TEMPLATE = "/sys/class/net/%s/speed";
};