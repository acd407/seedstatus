#include <modules/gpu.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <cmath>

GpuModule::GpuModule() : Module("gpu") {
    // GPU模块每秒钟更新一次
    setInterval(1);
}

GpuModule::~GpuModule() {}

void GpuModule::update() {
    try {
        // 获取GPU使用率
        uint64_t usage = getGpuUsage();

        // 构建输出字符串
        std::ostringstream output;
        output << "󰍹" << " ";

        // 根据当前状态显示GPU使用率或显存占用
        if (show_vram_) {
            // 显示显存占用
            uint64_t vram_used = getVramUsed();
            char vram_str[6];
            formatStorageUnits(vram_str, vram_used);
            output << vram_str;
        } else {
            // 显示GPU使用率
            output.width(usage < 100 ? 2 : 3);
            output << usage << "%";
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
        setOutput("󰍹 --.-", Color::DEACTIVE);
    }
}

void GpuModule::handleClick(uint64_t button) {
    switch (button) {
    case 3: { // 右键点击 - 切换显示模式（GPU使用率或显存占用）
        show_vram_ = !show_vram_;
        update();
        break;
    }
    default:
        // 其他点击不做处理
        break;
    }
}

uint64_t GpuModule::getGpuUsage() {
    return Module::readUint64File(GPU_USAGE);
}

uint64_t GpuModule::getVramUsed() {
    return Module::readUint64File(VRAM_USED);
}

void GpuModule::formatStorageUnits(char *buffer, uint64_t bytes) {
    const char *units[] = {"B", "K", "M", "G", "T", "P", "E"};
    size_t unit_idx = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unit_idx < sizeof(units) / sizeof(units[0]) - 1) {
        size /= 1024.0;
        unit_idx++;
    }

    // 格式化为两位小数
    snprintf(buffer, 6, "%.2f%s", size, units[unit_idx]);
}