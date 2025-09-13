#include <modules/audio.h>
#include <alsa/asoundlib.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <system_error>
#include <iomanip>
#include <unistd.h>
#include <optional>

// 现代化的ALSA混音器RAII包装器
class AlsaMixerWrapper {
  private:
    std::unique_ptr<snd_mixer_t, decltype(&snd_mixer_close)> handle_;
    snd_mixer_elem_t *elem_ = nullptr;
    std::string element_name_;

  public:
    explicit AlsaMixerWrapper(const std::string &element_name)
        : handle_(nullptr, &snd_mixer_close), element_name_(element_name) {}

    bool initialize() {
        snd_mixer_t *raw_handle = nullptr;
        int err;

        if ((err = snd_mixer_open(&raw_handle, 0)) < 0) {
            std::cerr << "无法打开混音器: " << snd_strerror(err) << std::endl;
            return false;
        }

        handle_.reset(raw_handle);

        if ((err = snd_mixer_attach(handle_.get(), "default")) < 0 ||
            (err = snd_mixer_selem_register(handle_.get(), nullptr, nullptr)) <
                0 ||
            (err = snd_mixer_load(handle_.get())) < 0) {
            std::cerr << "混音器初始化失败: " << snd_strerror(err) << std::endl;
            return false;
        }

        snd_mixer_selem_id_t *sid;
        snd_mixer_selem_id_alloca(&sid);
        snd_mixer_selem_id_set_index(sid, 0);
        snd_mixer_selem_id_set_name(sid, element_name_.c_str());

        elem_ = snd_mixer_find_selem(handle_.get(), sid);
        if (!elem_) {
            std::cerr << "未找到 " << element_name_ << " 控制元素" << std::endl;
            return false;
        }

        return true;
    }

    std::optional<int64_t> getPlaybackVolume() {
        if (!handle_ || !elem_)
            return std::nullopt;

        int unmuted = 1;
        if (snd_mixer_selem_has_playback_switch(elem_)) {
            snd_mixer_selem_get_playback_switch(
                elem_, SND_MIXER_SCHN_FRONT_LEFT, &unmuted
            );
        }

        if (!unmuted)
            return -1; // 静音

        int64_t min, max, volume;
        snd_mixer_selem_get_playback_volume_range(elem_, &min, &max);
        snd_mixer_selem_get_playback_volume(
            elem_, SND_MIXER_SCHN_FRONT_LEFT, &volume
        );

        if (max > min) {
            volume = (volume - min) * 100 / (max - min);
        } else {
            volume = 0;
        }

        return volume;
    }

    std::optional<int64_t> getCaptureVolume() {
        if (!handle_ || !elem_)
            return std::nullopt;

        int unmuted = 1;
        if (snd_mixer_selem_has_capture_switch(elem_)) {
            snd_mixer_selem_get_capture_switch(
                elem_, SND_MIXER_SCHN_FRONT_LEFT, &unmuted
            );
        }

        if (!unmuted)
            return -1; // 静音

        int64_t min, max, volume;
        snd_mixer_selem_get_capture_volume_range(elem_, &min, &max);
        snd_mixer_selem_get_capture_volume(
            elem_, SND_MIXER_SCHN_FRONT_LEFT, &volume
        );

        if (max > min) {
            volume = (volume - min) * 100 / (max - min);
        } else {
            volume = 0;
        }

        return volume;
    }

    void handleEvents() {
        if (handle_) {
            snd_mixer_handle_events(handle_.get());
        }
    }

    bool isValid() const {
        return handle_ && elem_;
    }

    snd_mixer_t *getHandle() const {
        return handle_.get();
    }
    int getFd() const {
        if (!handle_)
            return -1;
        struct pollfd pfd;
        if (snd_mixer_poll_descriptors(handle_.get(), &pfd, 1) == 1) {
            return pfd.fd;
        }
        return -1;
    }
};

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
      element_name_(element_name), mixer_elem_(nullptr), mixer_fd_(-1),
      mixer_wrapper_(std::make_unique<AlsaMixerWrapper>(element_name)) {
    // 音频模块默认不基于时间间隔更新，而是基于ALSA事件
    setInterval(0);
}

AudioModule::~AudioModule() = default;

void AudioModule::init() {
    if (!mixer_wrapper_->initialize()) {
        // 如果初始化失败，设置一个错误输出
        setOutput("󰝟", Color::DEACTIVE);
        // 设置一个时间间隔以便稍后重试
        setInterval(1);
        return;
    }

    // 获取ALSA混音器的文件描述符
    if (mixer_wrapper_->isValid()) {
        int fd = mixer_wrapper_->getFd();
        if (fd >= 0) {
            mixer_fd_ = fd;
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
        if (!mixer_wrapper_->isValid()) {
            // 如果混音器未初始化，尝试重新初始化
            if (!mixer_wrapper_->initialize()) {
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

    const auto executeCommand = [this](const std::string &cmd) {
        std::cerr << getName() << ": Executing: " << cmd << std::endl;
        int result = system((cmd + " &").c_str());
        std::cerr << getName() << ": Command result: " << result << std::endl;
    };

    switch (button) {
    case 2: // 中键点击 - 打开音量控制
        if (getName() == "volume") {
            executeCommand("pavucontrol -t 3");
        } else if (getName() == "microphone") {
            executeCommand("pavucontrol -t 4");
        }
        break;
    case 3: // 右键点击 - 切换静音
        if (getName() == "volume") {
            executeCommand("~/.bin/wm/volume t");
        } else if (getName() == "microphone") {
            executeCommand("~/.bin/wm/volume m t");
        }
        // ALSA事件会自动触发update()，无需轮询
        break;
    case 4: // 上滚 - 增加音量
        if (getName() == "volume") {
            executeCommand("~/.bin/wm/volume i");
        } else if (getName() == "microphone") {
            executeCommand("~/.bin/wm/volume m i");
        }
        break;
    case 5: // 下滚 - 减少音量
        if (getName() == "volume") {
            executeCommand("~/.bin/wm/volume d");
        } else if (getName() == "microphone") {
            executeCommand("~/.bin/wm/volume m d");
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

bool AudioModule::initializeMixer() {
    return mixer_wrapper_->initialize();
}

void AudioModule::cleanupMixer() {
    mixer_wrapper_.reset();
}

void AudioModule::handleMixerEvents() {
    mixer_wrapper_->handleEvents();
}

// VolumeModule实现
VolumeModule::VolumeModule() : AudioModule("volume", "Master") {}

int64_t VolumeModule::getVolume() {
    auto volume_opt = mixer_wrapper_->getPlaybackVolume();
    if (!volume_opt) {
        std::cerr << "Volume: Failed to get volume" << std::endl;
        return -2;
    }

    int64_t volume = *volume_opt;
    std::cerr << "Volume: " << volume << "%" << std::endl;

    // 检查静音状态
    if (volume == -1) {
        return -1; // 返回静音状态
    }

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
    auto volume_opt = mixer_wrapper_->getCaptureVolume();
    if (!volume_opt) {
        std::cerr << "Microphone: Failed to get volume" << std::endl;
        return -2;
    }

    int64_t volume = *volume_opt;
    std::cerr << "Microphone: " << volume << "%" << std::endl;

    // 检查静音状态
    if (volume == -1) {
        return -1; // 返回静音状态
    }

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
