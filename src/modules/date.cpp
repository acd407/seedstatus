#include <modules/date.h>
#include <time.h>
#include <iostream>

DateModule::DateModule() : Module("date") {
    // Date模块每秒钟更新一次
    setInterval(1);
}

DateModule::~DateModule() {}

void DateModule::update() {
    time_t raw_time;
    time(&raw_time);                             // 获取当前时间戳
    struct tm *time_info = localtime(&raw_time); // 转换为本地时间

    char output_str[80];
    strftime(output_str, sizeof(output_str), "%a\u2004%m/%d\u2004%H:%M:%S", time_info);

    setOutput(output_str, Color::IDLE);
}

void DateModule::handleClick(uint64_t button) {
    switch (button) {
    case 2: // 中键点击
        // 在后台运行qjournalctl
        system("qjournalctl &");
        break;
    default:
        // 其他点击不做处理
        break;
    }
}

void DateModule::init() {
    // Date模块的初始化工作
    // 这里可以添加一些初始化代码，如果需要的话
}