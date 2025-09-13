#pragma once
#include <string>
#include <memory>
#include <functional>
#include <cstdint>
#include <chrono>

/**
 * @file module.h
 * @brief 模块系统的核心定义
 *
 * 本文件定义了状态栏应用程序的模块化架构基础，包括：
 * - 颜色枚举和颜色字符串转换
 * - Module基类：所有状态栏模块的父类
 * - ModuleManager类：模块管理器，负责模块的生命周期管理
 *
 * 模块系统设计特点：
 * - 基于事件驱动的更新机制
 * - 支持文件描述符监控（epoll集成）
 * - 统一的输出格式和颜色管理
 * - 模块间解耦，易于扩展
 */

/**
 * @brief 状态栏模块显示颜色枚举
 *
 * 定义了状态栏模块可以使用的各种颜色，用于表示不同的状态。
 * 颜色值对应i3bar协议中的颜色代码。
 */
enum class Color {
    DEACTIVE = 0, // #6A6862 - 非活动状态（灰色）
    COOL,         // #729FCF - 冷静状态（蓝色）
    GOOD,         // #98BC37 - 良好状态（绿色）
    IDLE,         // #FCE8C3 - 空闲状态（浅橙色）
    WARNING,      // #FED06E - 警告状态（黄色）
    CRITICAL      // #F75341 - 严重状态（红色）
};

/**
 * @brief 获取颜色的字符串表示
 * @param color 颜色枚举值
 * @return 对应的十六进制颜色字符串（如"#729FCF"）
 */
std::string getColorString(Color color);

/**
 * @brief 模块基类
 *
 * 所有状态栏模块的父类，定义了模块的基本接口和通用功能。
 * 每个模块负责监控和显示特定类型的系统信息。
 *
 * 设计特点：
 * - 支持基于时间间隔的定期更新
 * - 支持基于文件描述符的事件驱动更新
 * - 统一的输出格式和颜色管理
 * - 支持点击事件处理
 * - 线程安全的设计
 *
 * 使用示例：
 * @code
 * class MyModule : public Module {
 * public:
 *     MyModule() : Module("mymodule") {}
 *     void update() override {
 *         // 获取系统信息并设置输出
 *         setOutput("Hello World", Color::GOOD);
 *     }
 * };
 * @endcode
 */
class Module {
  public:
    /**
     * @brief 构造函数
     * @param name 模块名称，必须唯一
     */
    explicit Module(const std::string &name);

    /**
     * @brief 虚析构函数
     */
    virtual ~Module() = default;

    // 删除拷贝构造和赋值操作
    Module(const Module &) = delete;
    Module &operator=(const Module &) = delete;

    // 允许移动操作
    Module(Module &&) = default;
    Module &operator=(Module &&) = default;

    /**
     * @brief 获取模块名称
     * @return 模块名称字符串
     */
    std::string getName() const;

    /**
     * @brief 获取模块输出
     * @return 当前输出内容
     */
    std::string getOutput() const;

    /**
     * @brief 设置模块输出
     * @param output 输出内容
     * @param color 输出颜色，默认为IDLE
     */
    void setOutput(const std::string &output, Color color = Color::IDLE);

    /**
     * @brief 设置更新间隔（秒）
     * @param interval 更新间隔，0表示禁用定时更新
     */
    void setInterval(uint64_t interval);

    /**
     * @brief 获取更新间隔
     * @return 更新间隔（秒）
     */
    uint64_t getInterval() const;

    /**
     * @brief 设置模块状态
     * @param state 状态值
     */
    void setState(uint64_t state);

    /**
     * @brief 获取模块状态
     * @return 状态值
     */
    uint64_t getState() const;

    /**
     * @brief 更新模块，子类必须实现
     *
     * 这是模块的核心方法，负责获取最新的系统信息并更新输出。
     * 可以基于时间间隔或事件触发调用。
     */
    virtual void update() = 0;

    /**
     * @brief 处理点击事件，子类可以重写
     * @param button 鼠标按钮编号（1=左键，2=中键，3=右键，4=上滚，5=下滚）
     */
    virtual void handleClick(uint64_t button);

    /**
     * @brief 初始化模块，子类可以重写
     *
     * 在模块被添加到系统后调用，用于执行一次性初始化操作。
     */
    virtual void init();

    /**
     * @brief 设置文件描述符（用于epoll）
     * @param fd 文件描述符
     *
     * 设置后，System类会将该fd添加到epoll监控中，
     * 当fd有可读事件时会触发模块更新。
     */
    void setFd(int fd);

    /**
     * @brief 获取文件描述符
     * @return 文件描述符，-1表示未设置
     */
    int getFd() const;

    /**
     * @brief 检查是否需要删除该模块
     * @return true如果模块被标记为删除
     */
    bool shouldDelete() const;

    /**
     * @brief 标记模块为待删除
     *
     * 标记后，ModuleManager会在适当时机移除该模块。
     */
    void markForDeletion();

    /**
     * @brief 转换为JSON格式的输出
     * @return JSON格式的字符串，符合i3bar协议
     */
    std::string toJson() const;

    /**
     * @brief 读取Uint64格式的文件内容（静态方法）
     * @param path 文件路径
     * @return 文件内容作为uint64_t值
     *
     * 便捷方法，用于读取/sys/目录下的系统信息文件。
     */
    static uint64_t readUint64File(const std::string &path);

    /**
     * @brief 获取最后更新时间
     * @return 最后更新时间点
     */
    std::chrono::steady_clock::time_point getLastUpdateTime() const;

    /**
     * @brief 检查是否需要更新（基于时间间隔）
     * @return true如果需要更新
     */
    bool needsUpdate() const;

  protected:
    /**
     * @brief 更新最后更新时间
     *
     * 通常在update()方法开始时调用，用于跟踪更新时间。
     */
    void updateLastUpdateTime();

  private:
    std::string name_;                                       ///< 模块名称
    std::string output_;                                     ///< 当前输出内容
    std::string color_;                                      ///< 当前颜色值
    uint64_t interval_ = 0;                                  ///< 更新间隔（秒）
    uint64_t state_ = 0;                                     ///< 模块状态
    int fd_ = -1;                                            ///< 文件描述符
    volatile bool should_delete_ = false;                    ///< 删除标记
    std::chrono::steady_clock::time_point last_update_time_; ///< 最后更新时间
};

/**
 * @brief 模块管理器
 *
 * 负责管理所有模块的生命周期，包括添加、删除、查找和输出管理。
 * 提供统一的接口来操作模块集合。
 *
 * 功能特点：
 * - 支持动态添加和删除模块
 * - 提供按名称和索引查找模块
 * - 统一的JSON输出格式
 * - 自动清理标记为删除的模块
 * - 支持STL风格的迭代器接口
 *
 * 使用示例：
 * @code
 * ModuleManager manager;
 * auto module = std::make_shared<MyModule>();
 * manager.addModule(module);
 * manager.outputModules(); // 输出所有模块
 * @endcode
 */
class ModuleManager {
  public:
    /**
     * @brief 默认构造函数
     */
    ModuleManager() = default;

    /**
     * @brief 析构函数
     */
    ~ModuleManager() = default;

    // 删除拷贝构造和赋值操作
    ModuleManager(const ModuleManager &) = delete;
    ModuleManager &operator=(const ModuleManager &) = delete;

    // 允许移动操作
    ModuleManager(ModuleManager &&) = default;
    ModuleManager &operator=(ModuleManager &&) = default;

    /**
     * @brief 添加模块
     * @param module 要添加的模块共享指针
     *
     * 模块会被添加到管理器的内部列表中，
     * 之后可以通过名称或索引访问。
     */
    void addModule(std::shared_ptr<Module> module);

    /**
     * @brief 获取模块总数
     * @return 模块数量
     */
    size_t getModuleCount() const;

    /**
     * @brief 通过索引获取模块
     * @param index 模块索引
     * @return 模块共享指针，如果索引无效返回nullptr
     */
    std::shared_ptr<Module> getModule(size_t index) const;

    /**
     * @brief 通过名称获取模块
     * @param name 模块名称
     * @return 模块共享指针，如果未找到返回nullptr
     */
    std::shared_ptr<Module> getModuleByName(const std::string &name) const;

    /**
     * @brief 输出所有模块的JSON
     *
     * 将所有模块的输出格式化为JSON数组，符合i3bar协议要求。
     * 输出到标准输出，供i3bar/swaybar等状态栏程序使用。
     */
    void outputModules() const;

    /**
     * @brief 移除标记为删除的模块
     *
     * 清理所有被shouldDelete()标记为删除的模块。
     * 通常在主循环中定期调用。
     */
    void removeMarkedModules();

    /**
     * @brief 获取所有模块（只读）
     * @return 模块向量的常量引用
     */
    const std::vector<std::shared_ptr<Module>> &getModules() const;

    /**
     * @brief 获取起始迭代器
     * @return 指向模块列表开头的迭代器
     */
    auto begin() const {
        return modules_.begin();
    }

    /**
     * @brief 获取结束迭代器
     * @return 指向模块列表末尾的迭代器
     */
    auto end() const {
        return modules_.end();
    }

  private:
    std::vector<std::shared_ptr<Module>> modules_; ///< 模块列表
};