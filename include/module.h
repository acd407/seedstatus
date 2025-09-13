#pragma once
#include <string>
#include <memory>
#include <functional>
#include <cstdint>
#include <chrono>

// 颜色定义
enum class Color {
    DEACTIVE = 0, // #6A6862
    COOL,        // #729FCF
    GOOD,        // #98BC37
    IDLE,        // #FCE8C3
    WARNING,     // #FED06E
    CRITICAL     // #F75341
};

// 获取颜色的字符串表示
std::string getColorString(Color color);

// 模块基类
class Module {
public:
    explicit Module(const std::string& name);
    virtual ~Module() = default;
    
    // 删除拷贝构造和赋值操作
    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;
    
    // 允许移动操作
    Module(Module&&) = default;
    Module& operator=(Module&&) = default;

    // 获取模块名称
    std::string getName() const;
    
    // 获取模块输出
    std::string getOutput() const;
    
    // 设置模块输出
    void setOutput(const std::string& output, Color color = Color::IDLE);
    
    // 设置更新间隔（秒）
    void setInterval(uint64_t interval);
    
    // 获取更新间隔
    uint64_t getInterval() const;
    
    // 设置模块状态
    void setState(uint64_t state);
    
    // 获取模块状态
    uint64_t getState() const;
    
    // 更新模块，子类需要实现
    virtual void update() = 0;
    
    // 处理点击事件，子类可以重写
    virtual void handleClick(uint64_t button);
    
    // 初始化模块，子类可以重写
    virtual void init();
    
    // 设置文件描述符（用于epoll）
    void setFd(int fd);
    
    // 获取文件描述符
    int getFd() const;
    
    // 检查是否需要删除该模块
    bool shouldDelete() const;
    void markForDeletion();
    
    // 转换为JSON格式的输出
    std::string toJson() const;

    // 读取Uint64格式的文件内容（静态方法，所有模块可共用）
    static uint64_t readUint64File(const std::string& path);
    
    // 获取最后更新时间
    std::chrono::steady_clock::time_point getLastUpdateTime() const;
    
    // 检查是否需要更新（基于时间间隔）
    bool needsUpdate() const;

protected:
    // 更新最后更新时间
    void updateLastUpdateTime();

private:
    std::string name_;
    std::string output_;
    std::string color_;
    uint64_t interval_ = 0;
    uint64_t state_ = 0;
    int fd_ = -1;
    volatile bool should_delete_ = false;
    std::chrono::steady_clock::time_point last_update_time_;
};

// 模块管理器
class ModuleManager {
public:
    ModuleManager() = default;
    ~ModuleManager() = default;
    
    // 删除拷贝构造和赋值操作
    ModuleManager(const ModuleManager&) = delete;
    ModuleManager& operator=(const ModuleManager&) = delete;
    
    // 允许移动操作
    ModuleManager(ModuleManager&&) = default;
    ModuleManager& operator=(ModuleManager&&) = default;
    
    // 添加模块
    void addModule(std::shared_ptr<Module> module);
    
    // 获取模块总数
    size_t getModuleCount() const;
    
    // 通过索引获取模块
    std::shared_ptr<Module> getModule(size_t index) const;
    
    // 通过名称获取模块
    std::shared_ptr<Module> getModuleByName(const std::string& name) const;
    
    // 输出所有模块的JSON
    void outputModules() const;
    
    // 移除标记为删除的模块
    void removeMarkedModules();
    
    // 获取所有模块（只读）
    const std::vector<std::shared_ptr<Module>>& getModules() const;
    
    // 迭代器支持
    auto begin() const { return modules_.begin(); }
    auto end() const { return modules_.end(); }
    
private:
    std::vector<std::shared_ptr<Module>> modules_;
};