#pragma once
#include "module.h"
#include <unistd.h>
#include <system.h>

// Stdin模块 - 处理来自标准输入的事件（如点击事件）
class StdinModule : public Module {
  public:
    StdinModule(System *system = nullptr);
    ~StdinModule();

    // 更新模块状态
    virtual void update() override;

    // 初始化模块
    virtual void init() override;

    // 处理点击事件
    virtual void handleClick(uint64_t button) override;

  private:
    // 设置文件描述符为非阻塞模式
    void setNonBlocking(int fd);

    // 读取并解析输入
    void parseInput();

    // System实例指针
    System *system_;
};