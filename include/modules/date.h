#pragma once
#include "module.h"

// Date模块显示当前日期和时间
class DateModule : public Module {
  public:
    DateModule();
    ~DateModule() override;

    // 更新模块信息
    void update() override;

    // 处理点击事件
    void handleClick(uint64_t button) override;

    // 初始化模块
    void init() override;
};