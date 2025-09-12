#pragma once
#include "module.h"

// Temp模块显示系统温度
class TempModule : public Module {
public:
    TempModule();
    ~TempModule() override;
    
    // 更新模块信息
    void update() override;
    
    // 初始化模块
    void init() override;
    
private:
    // 获取系统温度
    double getTemperature() const;
    
    // 获取温度对应的图标
    std::string getTemperatureIcon(double temp) const;
    
    // 获取温度对应的颜色
    Color getTemperatureColor(double temp) const;
};