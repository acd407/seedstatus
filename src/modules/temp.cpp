#include <modules/temp.h>
#include <fstream>
#include <string>
#include <iostream>
#include <stdexcept>
#include <sstream>   // 用于 std::ostringstream
#include <vector>    // 用于 std::vector
#include <algorithm> // 用于 std::clamp

TempModule::TempModule() : Module("temp") {
    // Temp模块每秒钟更新一次
    setInterval(1);
}

TempModule::~TempModule() {}

void TempModule::update() {
    try {
        double temp = getTemperature();

        std::string icon = getTemperatureIcon(temp);

        Color color = getTemperatureColor(temp);

        // 格式化输出字符串（使用ostringstream代替snprintf）
        std::ostringstream output;
        output << icon << "\u2004";

        // 根据温度设置精度
        output.precision(temp < 10 ? 2 : 1);
        output << std::fixed << temp;

        setOutput(output.str(), color);
    } catch (const std::exception &e) {

        setOutput("\u2004--.-", Color::DEACTIVE);
    }
}

void TempModule::init() {
    // Temp模块的初始化工作
    // 这里可以添加一些初始化代码，如果需要的话
}

double TempModule::getTemperature() const {
    const std::string temp_file = "/sys/class/hwmon/hwmon5/temp1_input";
    std::ifstream file(temp_file);

    if (!file.is_open()) {
        throw std::runtime_error(
            "Failed to open temperature file: " + temp_file
        );
    }

    uint64_t temp_raw;
    file >> temp_raw;

    if (!file) {
        throw std::runtime_error(
            "Failed to read temperature from file: " + temp_file
        );
    }

    // 将温度值从毫摄氏度转换为摄氏度
    return static_cast<double>(temp_raw) / 1000.0;
}

std::string TempModule::getTemperatureIcon(double temp) const {
    // 使用vector代替数组，更现代且易于扩展
    const std::vector<std::string> icons = {
        "", // 低温
        "", // 较低温度
        "", // 中等温度
        "", // 较高温度
        ""  // 高温
    };

    // 根据温度选择图标（改进算法，更合理地分布温度区间）
    // 假设温度范围为0-100°C，将其平均分为5个区间
    double normalizedTemp = std::clamp(temp, 0.0, 100.0);
    size_t idx = static_cast<size_t>((normalizedTemp / 100.0) * static_cast<double>(icons.size()));

    // 确保索引在有效范围内（使用clamp函数）
    idx = std::clamp(idx, size_t(0), icons.size() - 1);

    return icons[idx];
}

Color TempModule::getTemperatureColor(double temp) const {
    if (temp >= 80) {
        return Color::CRITICAL;
    } else if (temp >= 60) {
        return Color::WARNING;
    } else if (temp >= 30) {
        return Color::IDLE;
    } else {
        return Color::COOL;
    }
}