#pragma once
#include "module.h"
#include <alsa/asoundlib.h>
#include <memory>
#include <string>
#include <optional>

// 音频模块基类 - 提供ALSA混音器的通用功能
class AudioModule : public Module {
public:
    explicit AudioModule(const std::string& name, const std::string& element_name);
    virtual ~AudioModule();
    
    // 删除拷贝构造和赋值操作
    AudioModule(const AudioModule&) = delete;
    AudioModule& operator=(const AudioModule&) = delete;
    
    // 删除移动操作
    AudioModule(AudioModule&&) = delete;
    AudioModule& operator=(AudioModule&&) = delete;
    
    // 更新模块状态
    virtual void update() override;
    
    // 处理点击事件
    virtual void handleClick(uint64_t button) override;
    
    // 初始化模块
    virtual void init() override;
    
protected:
    // 获取音量值（子类需要实现具体的获取逻辑）
    virtual int64_t getVolume() = 0;
    
    // 获取音量图标（子类需要实现）
    virtual std::string getVolumeIcon(int64_t volume) = 0;
    
    // 格式化输出字符串（子类可以重写）
    virtual std::string formatOutput(int64_t volume);
    
    // 处理ALSA混音器事件
    void handleMixerEvents();
    
    // ALSA混音器句柄
    std::unique_ptr<snd_mixer_t, decltype(&snd_mixer_close)> mixer_handle_;
    
    // 混音器元素名称
    std::string element_name_;
    
    // 混音器元素
    snd_mixer_elem_t* mixer_elem_ = nullptr;
    
    // ALSA混音器文件描述符
    int mixer_fd_ = -1;
    
    // 现代化的ALSA混音器包装器
    std::unique_ptr<class AlsaMixerWrapper> mixer_wrapper_;
    
private:
    
    // 初始化ALSA混音器
    bool initializeMixer();
    
    // 清理ALSA资源
    void cleanupMixer();
};

// 音量模块 - 控制系统主音量
class VolumeModule : public AudioModule {
public:
    VolumeModule();
    ~VolumeModule() = default;
    
protected:
    // 获取音量值
    virtual int64_t getVolume() override;
    
    // 获取音量图标
    virtual std::string getVolumeIcon(int64_t volume) override;
    
    // 格式化输出字符串
    virtual std::string formatOutput(int64_t volume) override;
    
private:
    // 音量图标定义
    static const std::vector<std::string> volume_icons_;
};

// 麦克风模块 - 控制麦克风音量
class MicrophoneModule : public AudioModule {
public:
    MicrophoneModule();
    ~MicrophoneModule() = default;
    
protected:
    // 获取音量值
    virtual int64_t getVolume() override;
    
    // 获取音量图标
    virtual std::string getVolumeIcon(int64_t volume) override;
    
    // 格式化输出字符串
    virtual std::string formatOutput(int64_t volume) override;
    
private:
    // 麦克风图标定义
    static const std::vector<std::string> microphone_icons_;
};
