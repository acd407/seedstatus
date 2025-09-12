#include <modules/memory.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <iomanip>

MemoryModule::MemoryModule() : Module("memory") {
    // 内存模块每2秒更新一次
    setInterval(2);
}

MemoryModule::~MemoryModule() {}

void MemoryModule::update() {
    try {
        // 获取内存使用情况
        uint64_t used;
        double usage;
        getUsage(used, usage);
        
        // 构建输出字符串
        std::ostringstream output;
        output << "󰍛" << "\u2004"; // 内存图标和空格
        
        // 格式化内存使用量
        output << formatStorageUnits(static_cast<double>(used));
        
        // 选择颜色
        Color color = Color::IDLE;
        if (usage >= 80) {
            color = Color::CRITICAL;
        } else if (usage >= 50) {
            color = Color::WARNING;
        }
        
        // 设置输出
        setOutput(output.str(), color);
        
    } catch (const std::exception &e) {
        setOutput("󰍛\u2004--.-", Color::DEACTIVE);
    }
}

void MemoryModule::handleClick(uint64_t button) {
    switch (button) {
    case 3: { // 右键点击 - 切换显示模式（可以扩展功能）
        uint64_t old_state = getState();
        setState(old_state ^ 1);
        
        update();
        break;
    }
    default:
        // 其他点击不做处理
        break;
    }
}

void MemoryModule::getUsage(uint64_t& used, double& percent) {
    std::ifstream file(MEMINFO);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open " + std::string(MEMINFO));
    }
    
    std::string line;
    uint64_t total = 0;
    uint64_t available = 0;
    
    while (std::getline(file, line)) {
        if (line.find("MemTotal:") == 0) {
            // 提取数值（跳过"MemTotal:"和空格）
            size_t start = line.find_first_of("0123456789");
            if (start != std::string::npos) {
                size_t end = line.find_first_not_of("0123456789", start);
                std::string value_str = line.substr(start, end - start);
                total = std::stoull(value_str);
            }
        } else if (line.find("MemAvailable:") == 0) {
            // 提取数值（跳过"MemAvailable:"和空格）
            size_t start = line.find_first_of("0123456789");
            if (start != std::string::npos) {
                size_t end = line.find_first_not_of("0123456789", start);
                std::string value_str = line.substr(start, end - start);
                available = std::stoull(value_str);
            }
        }
    }
    
    file.close();
    
    if (total == 0) {
        throw std::runtime_error("Failed to get total memory");
    }
    
    used = total - available;
    percent = static_cast<double>(used) / total;
    used *= 1024; // 转换为字节
}

std::string MemoryModule::formatStorageUnits(double bytes) {
    const std::string units = "KMGTPE"; // 单位从 K 开始
    int unit_idx = -1;                   // 0=K, 1=M, 2=G, 3=T, 4=P, 5=E
    
    // 确保最小单位为 K
    while ((bytes >= 1000.0 && unit_idx < 5) || unit_idx < 0) {
        bytes /= 1024;
        unit_idx++;
    }
    
    // 动态选择格式
    std::ostringstream result;
    result << std::fixed;
    
    if (bytes >= 100.0) {
        result << std::setprecision(0) << " " << bytes << units[unit_idx];
    } else if (bytes >= 10.0) {
        result << std::setprecision(1) << std::setw(4) << bytes << units[unit_idx];
    } else {
        result << std::setprecision(2) << std::setw(4) << bytes << units[unit_idx];
    }
    
    return result.str();
}