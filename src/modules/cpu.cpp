#include <modules/cpu.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <algorithm> // 用于 std::clamp

CpuModule::CpuModule() : Module("cpu") {
    // CPU模块每秒钟更新一次
    setInterval(1);
}

CpuModule::~CpuModule() {}

void CpuModule::update() {
    try {
        // 获取CPU使用率
        double usage = getUsage();

        // 选择图标 (使用vector代替数组，更现代且易于扩展)
        const std::vector<std::string> icons = {"󰾆", "󰾅", "󰓅"};
        size_t icon_idx = icons.size() * usage / 101;
        icon_idx = std::clamp(icon_idx, size_t(0), icons.size() - 1);

        // 构建输出字符串 (使用stringstream代替C风格的snprintf)
        std::ostringstream output;
        output << icons[icon_idx] << " ";

        // 根据当前状态显示使用率或功率
        if (getState()) {
            // 显示功率
            double power = getPower();
            output.precision(power < 10 ? 2 : 1);
            output << std::fixed << power << "W";
        } else {
            // 显示使用率
            output.precision(usage < 10 ? 2 : 1);
            output << std::fixed << usage << "%";
        }

        // 选择颜色
        Color color = Color::IDLE;
        if (usage >= 60) {
            color = Color::CRITICAL;
        } else if (usage >= 30) {
            color = Color::WARNING;
        }

        // 设置输出
        setOutput(output.str(), color);

    } catch (const std::exception &e) {
        setOutput("󰓅 --.-", Color::DEACTIVE);
    }
}

void CpuModule::handleClick(uint64_t button) {
    switch (button) {
    case 3: { // 右键点击 - 切换显示模式（使用率或功率）
        // 使用大括号确保变量作用域仅限于此case分支
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

double CpuModule::getUsage() {
    std::ifstream file(PROC_STAT);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open " + std::string(PROC_STAT));
    }

    std::string line;
    if (!std::getline(file, line)) {
        throw std::runtime_error(
            "Failed to read from " + std::string(PROC_STAT)
        );
    }

    file.close();

    uint64_t idx, nice, system, idle, iowait, irq, softirq;
    if (sscanf(
            line.c_str(), "cpu %lu %lu %lu %lu %lu %lu %lu", &idx, &nice,
            &system, &idle, &iowait, &irq, &softirq
        ) != 7) {
        throw std::runtime_error("Failed to parse CPU stats");
    }

    uint64_t total = idx + nice + system + idle + iowait + irq + softirq;
    uint64_t total_idle = idle + iowait;
    uint64_t total_diff = total - prev_total_;
    uint64_t idle_diff = total_idle - prev_idle_;

    double cpu_usage = 0.0;
    if (prev_total_ != 0 && total_diff != 0) {
        cpu_usage = 100.0 * (total_diff - idle_diff) / total_diff;
    }

    prev_idle_ = total_idle;
    prev_total_ = total;

    return cpu_usage;
}

double CpuModule::getPower() {
    if (USE_RAPL) {
        uint64_t energy = Module::readUint64File(PACKAGE);

        if (prev_energy_ == 0) {
            prev_energy_ = energy;
            return 0.0;
        }

        uint64_t energy_diff = energy - prev_energy_;
        double power =
            static_cast<double>(energy_diff) / 1e6; // 转换为焦耳/秒（瓦）

        prev_energy_ = energy;
        return power;
    } else {
        uint64_t uwatt_core = Module::readUint64File(SVI2_P_Core);
        uint64_t uwatt_soc = Module::readUint64File(SVI2_P_SoC);
        return static_cast<double>(uwatt_core + uwatt_soc) / 1e6; // 转换为瓦
    }
}
