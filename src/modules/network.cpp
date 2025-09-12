#include <modules/network.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <sys/stat.h>
#include <cmath>

NetworkModule::NetworkModule() : Module("network") {
    // 网络模块每秒钟更新一次
    setInterval(1);
}

NetworkModule::~NetworkModule() {}

void NetworkModule::update() {
    try {
        std::ostringstream output;
        
        uint64_t rx = 0, tx = 0;
        std::string master_ifname;
        getNetworkSpeedAndMasterDev(rx, tx, master_ifname);
        
        if (master_ifname.empty()) {
            output << "󱞐";
        } else if (master_ifname[0] == 'e') {
            formatEtherOutput(output, master_ifname, rx, tx);
        } else if (master_ifname[0] == 'w') {
            formatWirelessOutput(output, master_ifname, rx, tx);
        } else {
            output << "󱞐";
        }
        
        // 设置输出
        setOutput(output.str(), Color::IDLE);
        
    } catch (const std::exception &e) {
        setOutput("󱞐", Color::DEACTIVE);
    }
}

void NetworkModule::handleClick(uint64_t button) {
    switch (button) {
    case 2: // 中键点击 - 打开网络管理工具
        system("iwgtk &");
        break;
    case 3: { // 右键点击 - 切换显示模式
        show_details_ = !show_details_;
        update();
        break;
    }
    default:
        // 其他点击不做处理
        break;
    }
}

void NetworkModule::getWirelessStatus(const std::string& ifname, int64_t& link, int64_t& level) {
    std::ifstream file(WIRELESS_STATUS);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open " + std::string(WIRELESS_STATUS));
    }
    
    std::string line;
    // 跳过前两行
    std::getline(file, line);
    std::getline(file, line);
    
    link = 0;
    level = 0;
    
    while (std::getline(file, line)) {
        // 跳过前导空格
        size_t start = line.find_first_not_of(" ");
        if (start == std::string::npos) continue;
        
        std::string current_line = line.substr(start);
        
        // 检查是否匹配接口名称
        if (current_line.compare(0, ifname.length(), ifname) != 0) {
            continue;
        }
        
        // 查找冒号位置
        size_t colon_pos = current_line.find(':');
        if (colon_pos == std::string::npos) continue;
        
        std::string data_part = current_line.substr(colon_pos + 1);
        
        // 解析链路质量和信号级别
        if (sscanf(data_part.c_str(), "%*d %ld. %ld. %*d", &link, &level) == 2) {
            break;
        }
    }
    
    file.close();
    
    // rtw88 驱动程序链路质量最大值是 70，不是 100
    link = link * 10 / 7;
}

void NetworkModule::getNetworkSpeedAndMasterDev(uint64_t& rx, uint64_t& tx, std::string& master) {
    std::ifstream file(NET_DEV);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open " + std::string(NET_DEV));
    }
    
    std::string line;
    // 跳过前两行
    std::getline(file, line);
    std::getline(file, line);
    
    bool found = false;
    rx = 0;
    tx = 0;
    
    while (std::getline(file, line)) {
        // 跳过前导空格
        size_t start = line.find_first_not_of(" ");
        if (start == std::string::npos) continue;
        
        std::string current_line = line.substr(start);
        
        // 查找冒号位置
        size_t colon_pos = current_line.find(':');
        if (colon_pos == std::string::npos) continue;
        
        std::string ifname = current_line.substr(0, colon_pos);
        std::string data_part = current_line.substr(colon_pos + 1);
        
        // 只接受 wlan 和 ether
        if (ifname.empty() || (ifname[0] != 'w' && ifname[0] != 'e')) {
            continue;
        }
        
        // 检查网络接口是否 up
        char carrier_path[256];
        snprintf(carrier_path, sizeof(carrier_path), CARRIER_PATH_TEMPLATE, ifname.c_str());
        
        struct stat st;
        if (stat(carrier_path, &st) != 0) {
            continue;
        }
        
        std::ifstream carrier_file(carrier_path);
        if (!carrier_file.is_open()) {
            continue;
        }
        
        uint64_t carrier;
        carrier_file >> carrier;
        carrier_file.close();
        
        if (!carrier) {
            continue;
        }
        
        // 保存接口名称
        if (!found || ifname[0] == 'e') {
            master = ifname;
        }
        
        // 解析接收和发送字节数
        uint64_t prx, ptx;
        if (sscanf(data_part.c_str(), "%lu %*u %*u %*u %*u %*u %*u %*u %lu", &prx, &ptx) == 2) {
            rx += prx;
            tx += ptx;
            found = true;
        }
    }
    
    file.close();
    
    if (!found) {
        master = "";
        return;
    }
    
    // 计算差值
    if (prev_rx_ == 0) {
        prev_rx_ = rx;
    }
    if (prev_tx_ == 0) {
        prev_tx_ = tx;
    }
    
    uint64_t rx_diff = rx - prev_rx_;
    uint64_t tx_diff = tx - prev_tx_;
    
    prev_rx_ = rx;
    prev_tx_ = tx;
    
    rx = rx_diff;
    tx = tx_diff;
}

void NetworkModule::formatEtherOutput(std::ostringstream& output, const std::string& ifname, uint64_t rx, uint64_t tx) {
    if (show_details_) {
        // 显示网络速度
        char speed_path[256];
        snprintf(speed_path, sizeof(speed_path), SPEED_PATH_TEMPLATE, ifname.c_str());
        
        try {
            uint64_t speed = Module::readUint64File(speed_path);
            // 检查是否为无效值（-1在无符号中表示很大的数）
            if (speed == static_cast<uint64_t>(-1) || speed > 10000) {
                output << "󰈀" << " " << "--M";
            } else {
                output << "󰈀" << " " << speed << "M";
            }
        } catch (const std::exception& e) {
            // 读取失败时显示--
            output << "󰈀" << " " << "--M";
        }
    } else {
        // 显示流量
        output << "󰈀" << " ";
        formatStorageUnits(output, rx);
        output << " ";
        formatStorageUnits(output, tx);
    }
}

void NetworkModule::formatWirelessOutput(std::ostringstream& output, const std::string& ifname, uint64_t rx, uint64_t tx) {
    int64_t link = 0, level = 0;
    getWirelessStatus(ifname, link, level);
    
    const std::vector<std::string> icons = {"󰤮", "󰤯", "󰤟", "󰤢", "󰤥", "󰤨"};
    size_t idx = 0;
    idx += level > -100;
    idx += level > -90;
    idx += level > -80;
    idx += level > -65;
    idx += level > -55;
    
    // 采用 level 方法判断出问题的时候，就用 link 方法
    if (idx == 0 || idx >= icons.size()) {
        idx = icons.size() * link / 101;
    }
    
    idx = std::min(idx, icons.size() - 1);
    
    if (show_details_) {
        output << icons[idx] << " " << link << "%" << " " << level << "dB";
    } else {
        output << icons[idx] << " ";
        formatStorageUnits(output, rx);
        output << " ";
        formatStorageUnits(output, tx);
    }
}

void NetworkModule::formatStorageUnits(std::ostringstream& output, uint64_t bytes) {
    const char* units = "KMGTPE"; // 单位从 K 开始
    int unit_idx = -1;            // 0=K, 1=M, 2=G, 3=T, 4=P, 5=E
    
    double size = static_cast<double>(bytes);
    
    // 确保最小单位为 K
    while ((size >= 1000.0 && unit_idx < 5) || unit_idx < 0) {
        size /= 1024;
        unit_idx++;
    }
    
    // 动态选择格式，确保不超过5个字符
    if (size >= 100.0) {
        output.precision(0);
        output << std::fixed << " " << std::round(size) << units[unit_idx];
    } else if (size >= 10.0) {
        output.precision(1);
        output << std::fixed << size << units[unit_idx];
    } else {
        output.precision(2);
        output << std::fixed << size << units[unit_idx];
    }
}