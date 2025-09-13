#include "modules/audio.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <system_error>
#include <iomanip>
#include <unistd.h>

// 音量模块的图标定义
const std::vector<std::string> VolumeModule::volume_icons_ = {
    "󰕿", "󰖀", "󰕾", "󰝝"
};

// 麦克风模块的图标定义
const std::vector<std::string> MicrophoneModule::microphone_icons_ = {
    "󰍮", "󰢳", "󰍬", "󰢴"
};

// AudioModule基类实现
AudioModule::AudioModule(
    const std::string &name, const std::string &element_name
)
    : Module(name), mixer_handle_(nullptr, &snd_mixer_close),
      element_name_(element_name) {
    // 音频模块默认不基于时间间隔更新，而是基于ALSA事件
    setInterval(0);
}

AudioModule::~AudioModule() {
    cleanupMixer();
}

void AudioModule::init() {
    if (!initializeMixer()) {
        // 如果初始化失败，设置一个错误输出
        setOutput("󰝟", Color::DEACTIVE);
        // 设置一个时间间隔以便稍后重试
        setInterval(1);
        return;
    }

    // 获取ALSA混音器的文件描述符
    if (mixer_handle_) {
        struct pollfd pfd;
        if (snd_mixer_poll_descriptors(mixer_handle_.get(), &pfd, 1) == 1) {
            mixer_fd_ = pfd.fd;
            setFd(mixer_fd_); // 设置文件描述符，System会将其添加到epoll
            std::cerr << "AudioModule " << getName() << " registered fd "
                      << mixer_fd_ << " for epoll" << std::endl;
        } else {
            std::cerr << "Failed to get poll descriptors for " << getName()
                      << std::endl;
        }
    }
}

void AudioModule::update() {
    try {
        if (!mixer_handle_) {
            // 如果混音器未初始化，尝试重新初始化
            if (!initializeMixer()) {
                setOutput("󰝟", Color::DEACTIVE);
                setInterval(1); // 设置重试间隔
                return;
            }
        }

        // 处理ALSA混音器事件
        handleMixerEvents();

        // 获取音量值
        int64_t volume = getVolume();

        if (volume == -2) {
            // 设备未激活，需要重新加载
            cleanupMixer();
            setOutput("󰝟", Color::DEACTIVE);
            setInterval(1); // 设置重试间隔
        } else if (volume == -1) {
            // 静音状态
            setOutput(getVolumeIcon(volume), Color::IDLE);
        } else {
            // 正常状态，格式化输出
            std::string output = formatOutput(volume);
            setOutput(output, Color::IDLE);
            setInterval(0); // 取消时间间隔更新
        }
    } catch (const std::exception &e) {
        std::cerr << "AudioModule update error: " << e.what() << std::endl;
        setOutput("󰝟", Color::DEACTIVE);
        setInterval(1); // 设置重试间隔
    }
}

void AudioModule::handleClick(uint64_t button) {
    (void)button; // 避免未使用参数警告

    switch (button) {
    case 2: // 中键点击 - 打开音量控制
        if (getName() == "volume") {
            system("pavucontrol -t 3 &");
        } else if (getName() == "microphone") {
            system("pavucontrol -t 4 &");
        }
        break;
    case 3: // 右键点击 - 切换静音
        if (getName() == "volume") {
            std::cerr << "Volume: Toggling mute via system command"
                      << std::endl;
            int result = system("~/.bin/wm/volume t &");
            std::cerr << "Volume: System command result: " << result
                      << std::endl;
        } else if (getName() == "microphone") {
            std::cerr << "Microphone: Toggling mute via system command"
                      << std::endl;
            int result = system("~/.bin/wm/volume m t &");
            std::cerr << "Microphone: System command result: " << result
                      << std::endl;
        }
        // ALSA事件会自动触发update()，无需轮询
        break;
    case 4: // 上滚 - 增加音量
        if (getName() == "volume") {
            std::cerr << "Volume: Increasing volume via system command"
                      << std::endl;
            int result = system("~/.bin/wm/volume i &");
            std::cerr << "Volume: System command result: " << result
                      << std::endl;
        } else if (getName() == "microphone") {
            std::cerr << "Microphone: Increasing volume via system command"
                      << std::endl;
            int result = system("~/.bin/wm/volume m i &");
            std::cerr << "Microphone: System command result: " << result
                      << std::endl;
        }
        break;
    case 5: // 下滚 - 减少音量
        if (getName() == "volume") {
            std::cerr << "Volume: Decreasing volume via system command"
                      << std::endl;
            int result = system("~/.bin/wm/volume d &");
            std::cerr << "Volume: System command result: " << result
                      << std::endl;
        } else if (getName() == "microphone") {
            std::cerr << "Microphone: Decreasing volume via system command"
                      << std::endl;
            int result = system("~/.bin/wm/volume m d &");
            std::cerr << "Microphone: System command result: " << result
                      << std::endl;
        }
        break;
    default:
        break;
    }
}

std::string AudioModule::formatOutput(int64_t volume) {
    std::ostringstream output;
    output << getVolumeIcon(volume) << " ";

    if (volume > 0) {
        if (volume < 10) {
            output << volume << "%";
        } else {
            output << std::setw(2) << volume << "%";
        }
    }

    return output.str();
}

void AudioModule::reloadMixer() {
    cleanupMixer();
    initializeMixer();
}

bool AudioModule::initializeMixer() {
    snd_mixer_t *handle = nullptr;
    int err;

    // 打开默认混音器
    if ((err = snd_mixer_open(&handle, 0)) < 0) {
        std::cerr << "无法打开混音器: " << snd_strerror(err) << std::endl;
        return false;
    }

    // 加载混音器
    if ((err = snd_mixer_attach(handle, "default")) < 0 ||
        (err = snd_mixer_selem_register(handle, NULL, NULL)) < 0 ||
        (err = snd_mixer_load(handle)) < 0) {
        std::cerr << "混音器初始化失败: " << snd_strerror(err) << std::endl;
        snd_mixer_close(handle);
        return false;
    }

    // 查找混音器元素
    snd_mixer_selem_id_t *sid;
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, element_name_.c_str());

    mixer_elem_ = snd_mixer_find_selem(handle, sid);
    if (!mixer_elem_) {
        std::cerr << "未找到 " << element_name_ << " 控制元素" << std::endl;
        snd_mixer_close(handle);
        return false;
    }

    // 保存混音器句柄
    mixer_handle_.reset(handle);

    return true;
}

void AudioModule::cleanupMixer() {
    mixer_handle_.reset();
    mixer_elem_ = nullptr;
}

void AudioModule::handleMixerEvents() {
    if (mixer_handle_) {
        snd_mixer_handle_events(mixer_handle_.get());
    }
}

// VolumeModule实现
VolumeModule::VolumeModule() : AudioModule("volume", "Master") {}

int64_t VolumeModule::getVolume() {
    if (!mixer_handle_ || !mixer_elem_) {
        std::cerr << "Volume: Mixer handle or element is null" << std::endl;
        return -2;
    }

    int unmuted = 1;
    if (snd_mixer_selem_has_playback_switch(mixer_elem_)) {
        snd_mixer_selem_get_playback_switch(
            mixer_elem_, SND_MIXER_SCHN_FRONT_LEFT, &unmuted
        );
        std::cerr << "Volume: Playback switch state: " << unmuted << std::endl;
    } else {
        std::cerr << "Volume: No playback switch available" << std::endl;
    }

    if (!unmuted) {
        std::cerr << "Volume: Muted, returning -1" << std::endl;
        return -1; // 静音
    }

    int64_t min, max, volume;
    snd_mixer_selem_get_playback_volume_range(mixer_elem_, &min, &max);
    snd_mixer_selem_get_playback_volume(
        mixer_elem_, SND_MIXER_SCHN_FRONT_LEFT, &volume
    );

    std::cerr << "Volume: min=" << min << ", max=" << max << ", raw=" << volume
              << std::endl;

    // 转换为0-100范围
    if (max > min) {
        volume = (volume - min) * 100 / (max - min);
    } else {
        volume = 0;
    }

    std::cerr << "Volume: Calculated percentage=" << volume << std::endl;

    return (volume + 1) / 5; // 返回0-20范围的值，用于图标选择
}

std::string VolumeModule::getVolumeIcon(int64_t volume) {
    if (volume == -1) {
        return "󰸈"; // 静音图标
    }

    // 将0-20映射到图标索引
    size_t icon_index = 0;
    if (volume > 0) {
        icon_index = std::min(
            static_cast<size_t>((volume + 4) / 5), volume_icons_.size() - 1
        );
    }

    return volume_icons_[icon_index];
}

std::string VolumeModule::formatOutput(int64_t volume) {
    std::ostringstream output;
    output << getVolumeIcon(volume) << " ";

    if (volume > 0) {
        int actual_volume = static_cast<int>(volume * 5); // 转换回0-100范围
        if (actual_volume < 10) {
            output << actual_volume << "%";
        } else if (actual_volume < 100) {
            output << std::setw(2) << actual_volume << "%";
        } else {
            output << std::setw(3) << actual_volume << "%";
        }
    }

    return output.str();
}

// MicrophoneModule实现
MicrophoneModule::MicrophoneModule() : AudioModule("microphone", "Capture") {}

int64_t MicrophoneModule::getVolume() {
    if (!mixer_handle_ || !mixer_elem_) {
        std::cerr << "Microphone: Mixer handle or element is null" << std::endl;
        return -2;
    }

    int unmuted = 1;
    if (snd_mixer_selem_has_capture_switch(mixer_elem_)) {
        snd_mixer_selem_get_capture_switch(
            mixer_elem_, SND_MIXER_SCHN_FRONT_LEFT, &unmuted
        );
        std::cerr << "Microphone: Capture switch state: " << unmuted
                  << std::endl;
    } else {
        std::cerr << "Microphone: No capture switch available" << std::endl;
    }

    if (!unmuted) {
        std::cerr << "Microphone: Muted, returning -1" << std::endl;
        return -1; // 静音
    }

    int64_t min, max, volume;
    snd_mixer_selem_get_capture_volume_range(mixer_elem_, &min, &max);
    snd_mixer_selem_get_capture_volume(
        mixer_elem_, SND_MIXER_SCHN_FRONT_LEFT, &volume
    );

    std::cerr << "Microphone: min=" << min << ", max=" << max
              << ", raw=" << volume << std::endl;

    // 转换为0-100范围
    if (max > min) {
        volume = (volume - min) * 100 / (max - min);
    } else {
        volume = 0;
    }

    std::cerr << "Microphone: Calculated percentage=" << volume << std::endl;

    return (volume + 1) / 5; // 返回0-20范围的值，用于图标选择
}

std::string MicrophoneModule::getVolumeIcon(int64_t volume) {
    if (volume == -1) {
        return "󰍭"; // 静音图标
    }

    // 将0-20映射到图标索引
    size_t icon_index = 0;
    if (volume > 0) {
        icon_index = std::min(
            static_cast<size_t>((volume + 4) / 5), microphone_icons_.size() - 1
        );
    }

    return microphone_icons_[icon_index];
}

std::string MicrophoneModule::formatOutput(int64_t volume) {
    std::ostringstream output;
    output << getVolumeIcon(volume) << " ";

    if (volume > 0) {
        int actual_volume = static_cast<int>(volume * 5); // 转换回0-100范围
        if (actual_volume < 10) {
            output << actual_volume << "%";
        } else if (actual_volume < 100) {
            output << std::setw(2) << actual_volume << "%";
        } else {
            output << std::setw(3) << actual_volume << "%";
        }
    }

    return output.str();
}