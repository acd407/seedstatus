#include "modules/battery.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <system_error>
#include <cstring>

BatteryModule::BatteryModule() : Module("battery"), detailed_mode_(false) {
    // 电池模块默认不基于时间间隔更新，而是基于DBus事件
    setInterval(0);
}

BatteryModule::~BatteryModule() {
    // sdbus-c++连接会自动清理
}

void BatteryModule::init() {
    try {
        setupDBusConnection();
        setupDBusMonitoring();

        // 获取DBus文件描述符并添加到epoll
        auto pollData = connection_->getEventLoopPollData();
        int dbus_fd = pollData.fd;

        if (dbus_fd == -1) {
            throw std::runtime_error("Failed to get DBus file descriptor");
        }

        // 设置文件描述符，System会将其添加到epoll
        setFd(dbus_fd);
        std::cerr << "BatteryModule registered fd " << dbus_fd << " for epoll" << std::endl;

        // 立即更新一次
        update();
    } catch (const std::exception &e) {
        std::cerr << "BatteryModule init error: " << e.what() << std::endl;
        setOutput("󱠵", Color::DEACTIVE);
        setInterval(1); // 设置重试间隔
    }
}

void BatteryModule::setupDBusConnection() {
    try {
        // 创建系统总线连接
        connection_ = sdbus::createSystemBusConnection();
        std::cerr << "BatteryModule: Successfully created DBus system connection" << std::endl;

        // 创建UPower电池设备的代理
        sdbus::ServiceName upowerService{UPOWER_SERVICE};
        sdbus::ObjectPath batteryPath{BATTERY_PATH};
        upowerProxy_ =
            sdbus::createProxy(*connection_, std::move(upowerService), std::move(batteryPath));
        std::cerr << "BatteryModule: Successfully created UPower proxy" << std::endl;
    } catch (const sdbus::Error &e) {
        std::cerr << "BatteryModule: Failed to setup DBus connection: " << e.getMessage()
                  << std::endl;
        throw;
    } catch (const std::exception &e) {
        std::cerr << "BatteryModule: Exception during DBus connection setup: " << e.what()
                  << std::endl;
        throw;
    }
}

void BatteryModule::setupDBusMonitoring() {
    try {
        // 注册PropertiesChanged信号处理
        upowerProxy_->uponSignal("PropertiesChanged")
            .onInterface("org.freedesktop.DBus.Properties")
            .call([this](
                      const std::string &interfaceName,
                      const std::map<std::string, sdbus::Variant> &changedProperties,
                      const std::vector<std::string> &invalidatedProperties
                  ) {
                this->onPropertiesChanged(interfaceName, changedProperties, invalidatedProperties);
            });
        std::cerr << "BatteryModule: Successfully registered PropertiesChanged signal handler"
                  << std::endl;

        // 注册Changed信号处理
        upowerProxy_->uponSignal("Changed")
            .onInterface("org.freedesktop.UPower.Device")
            .call([this]() { this->onDeviceChanged(); });
        std::cerr << "BatteryModule: Successfully registered Changed signal handler" << std::endl;
    } catch (const sdbus::Error &e) {
        std::cerr << "BatteryModule: Failed to setup DBus monitoring: " << e.getMessage()
                  << std::endl;
        throw;
    } catch (const std::exception &e) {
        std::cerr << "BatteryModule: Exception during DBus monitoring setup: " << e.what()
                  << std::endl;
        throw;
    }
}

void BatteryModule::update() {
    try {
        // 处理所有待处理的DBus事件
        if (connection_) {
            while (connection_->processPendingEvent()) {
                // 处理所有待处理的事件
            }
        }

        // 获取电池信息
        BatteryState state = BatteryState::UNKNOWN;
        double percentage = 0.0;
        int64_t time = -1;
        double energy = 0.0;
        double energy_rate = 0.0;

        getBatteryInfo(state, percentage, time, energy, energy_rate);

        // 格式化输出
        std::string output =
            formatOutput(state, static_cast<uint64_t>(percentage), energy, energy_rate, time);

        // 设置输出
        setOutput(output, Color::IDLE);

    } catch (const std::exception &e) {
        std::cerr << "BatteryModule update error: " << e.what() << std::endl;
        setOutput("󱠵", Color::DEACTIVE);
        setInterval(1); // 设置重试间隔
    }
}

void BatteryModule::handleClick(uint64_t button) {
    switch (button) {
    case 2: // 中键 - 打开电源统计
        std::cerr << "Battery: Opening power statistics" << std::endl;
        system("gnome-power-statistics >/dev/null 2>&1 &");
        break;
    case 3: // 右键 - 切换显示模式
        std::cerr << "Battery: Toggling display mode" << std::endl;
        detailed_mode_ = !detailed_mode_;
        update();
        break;
    default:
        break;
    }
}

void BatteryModule::onPropertiesChanged(
    const std::string &interfaceName,
    const std::map<std::string, sdbus::Variant> &changedProperties,
    const std::vector<std::string> &invalidatedProperties
) {
    (void)invalidatedProperties;

    if (interfaceName == DEVICE_INTERFACE) {
        std::cerr << "BatteryModule: Properties changed on " << interfaceName << std::endl;
        // 检查是否有我们关心的属性变化
        for (const auto &[name, value] : changedProperties) {
            if (name == "Percentage" || name == "State" || name == "Energy" ||
                name == "EnergyRate" || name == "TimeToFull" || name == "TimeToEmpty") {
                std::cerr << "BatteryModule: Relevant property changed: " << name << std::endl;
                // 触发更新
                update();
                break;
            }
        }
    }
}

void BatteryModule::onDeviceChanged() {
    std::cerr << "BatteryModule: Device state changed" << std::endl;
    // 设备状态变化，触发更新
    update();
}

void BatteryModule::getBatteryInfo(
    BatteryState &state, double &percentage, int64_t &time, double &energy, double &energy_rate
) {
    if (!upowerProxy_) {
        throw std::runtime_error("DBus proxy not available");
    }

    try {
        // 获取电池状态
        auto stateVar = upowerProxy_->getProperty("State").onInterface(DEVICE_INTERFACE);
        uint32_t state_uint = stateVar.get<uint32_t>();
        state = static_cast<BatteryState>(state_uint);

        // 获取电池信息
        auto percentageVar = upowerProxy_->getProperty("Percentage").onInterface(DEVICE_INTERFACE);
        percentage = percentageVar.get<double>();

        auto energyVar = upowerProxy_->getProperty("Energy").onInterface(DEVICE_INTERFACE);
        energy = energyVar.get<double>();

        auto energyRateVar = upowerProxy_->getProperty("EnergyRate").onInterface(DEVICE_INTERFACE);
        energy_rate = energyRateVar.get<double>();

        // 根据状态获取时间信息
        switch (state) {
        case BatteryState::CHARGING:
            try {
                auto timeToFullVar =
                    upowerProxy_->getProperty("TimeToFull").onInterface(DEVICE_INTERFACE);
                time = timeToFullVar.get<int64_t>();
                std::cerr << "BatteryModule: Time to full: " << time << " seconds" << std::endl;
            } catch (...) {
                std::cerr << "BatteryModule: Failed to get time to full, setting to -1"
                          << std::endl;
                time = -1;
            }
            break;
        case BatteryState::DISCHARGING:
            try {
                auto timeToEmptyVar =
                    upowerProxy_->getProperty("TimeToEmpty").onInterface(DEVICE_INTERFACE);
                time = timeToEmptyVar.get<int64_t>();
                std::cerr << "BatteryModule: Time to empty: " << time << " seconds" << std::endl;
            } catch (...) {
                std::cerr << "BatteryModule: Failed to get time to empty, setting to -1"
                          << std::endl;
                time = -1;
            }
            break;
        default:
            std::cerr << "BatteryModule: Battery state not charging/discharging, setting time to -1"
                      << std::endl;
            time = -1;
            break;
        }

    } catch (const sdbus::Error &e) {
        throw std::runtime_error(
            "Failed to get battery properties: " + std::string(e.getMessage())
        );
    } catch (const std::exception &e) {
        throw std::runtime_error("Error extracting property value: " + std::string(e.what()));
    }
}

std::string BatteryModule::getBatteryIcon(BatteryState state, uint64_t percentage) {
    switch (state) {
    case BatteryState::CHARGING:
    case BatteryState::FULLY_CHARGED: {
        if (charging_icons_.empty()) {
            return "󰂅";
        }
        size_t idx = charging_icons_.size() * percentage / 101;
        if (idx >= charging_icons_.size()) {
            idx = charging_icons_.size() - 1;
        }
        return charging_icons_[idx];
    }
    case BatteryState::DISCHARGING:
    case BatteryState::EMPTY: {
        if (discharging_icons_.empty()) {
            return "󰁹";
        }
        size_t idx = discharging_icons_.size() * percentage / 101;
        if (idx >= discharging_icons_.size()) {
            idx = discharging_icons_.size() - 1;
        }
        return discharging_icons_[idx];
    }
    default: // 同步中或未知状态
        return "󱠵";
    }
}

std::string BatteryModule::formatTime(int64_t seconds) {
    if (seconds <= 0) {
        return "";
    }

    uint64_t hours = static_cast<uint64_t>(seconds) / 3600;
    uint64_t minutes = (static_cast<uint64_t>(seconds) - hours * 3600) / 60;

    std::ostringstream oss;
    oss << "(" << hours << ":" << std::setw(2) << std::setfill('0') << minutes << ")";
    return oss.str();
}

std::string BatteryModule::formatOutput(
    BatteryState state, uint64_t percentage, double energy, double energy_rate, int64_t time
) {
    std::ostringstream output;

    // 确定颜色
    Color color = Color::IDLE;
    if (percentage < 20) {
        color = Color::CRITICAL;
    } else if (percentage < 40) {
        color = Color::WARNING;
    }

    // 输出图标
    std::string icon = getBatteryIcon(state, percentage);
    if (state == BatteryState::CHARGING || state == BatteryState::FULLY_CHARGED) {
        output << "<span color='" << getColorString(Color::GOOD) << "'>" << icon << "</span>";
    } else if (state == BatteryState::UNKNOWN || state == BatteryState::PENDING_CHARGE ||
               state == BatteryState::PENDING_DISCHARGE) {
        output << "<span color='" << getColorString(Color::DEACTIVE) << "'>" << icon << "</span>";
    } else {
        output << "<span color='" << getColorString(color) << "'>" << icon << "</span>";
    }

    // 输出文字信息
    if (detailed_mode_) {
        // 详细模式：显示能量信息
        output << "\u2004<span color='" << getColorString(color) << "'>" << std::fixed
               << std::setprecision(1) << energy << "Wh</span>";
        if (time > 0) {
            output << "\u2004(" << std::fixed << std::setprecision(1) << energy_rate << "W)";
        }
    } else {
        // 简单模式：显示百分比
        output << "\u2004<span color='" << getColorString(color) << "'>" << percentage
               << "%</span>";
        if (time > 0) {
            output << "\u2004" << formatTime(time);
        }
    }

    return output.str();
}