#include "modules/battery.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <system_error>
#include <cstring>

BatteryModule::BatteryModule()
    : Module("battery"), dbus_conn_(nullptr), detailed_mode_(false) {
    // 电池模块默认不基于时间间隔更新，而是基于DBus事件
    setInterval(0);
}

BatteryModule::~BatteryModule() {
    // 清理DBus连接
    if (dbus_conn_) {
        dbus_connection_unref(dbus_conn_);
    }
}

void BatteryModule::init() {
    // 初始化DBus错误
    DBusError err;
    dbus_error_init(&err);

    // 访问系统总线
    dbus_conn_ = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        std::cerr << "Failed to get DBus connection: " << err.message
                  << std::endl;
        dbus_error_free(&err);
        setOutput("󱠵", Color::DEACTIVE);
        setInterval(1); // 设置重试间隔
        return;
    }

    // 添加D-Bus信号匹配规则
    std::string match_rule =
        "type='signal',interface='org.freedesktop.DBus.Properties',"
        "path='" +
        BATTERY_PATH + "',arg0='" + DEVICE_INTERFACE + "'";
    dbus_bus_add_match(dbus_conn_, match_rule.c_str(), &err);
    if (dbus_error_is_set(&err)) {
        std::cerr << "Failed to add DBus match rule: " << err.message
                  << std::endl;
        dbus_error_free(&err);
        dbus_connection_unref(dbus_conn_);
        dbus_conn_ = nullptr;
        setOutput("󱠵", Color::DEACTIVE);
        setInterval(1); // 设置重试间隔
        return;
    }

    // 获取DBus文件描述符并添加到epoll
    int dbus_fd;
    if (!dbus_connection_get_unix_fd(dbus_conn_, &dbus_fd)) {
        std::cerr << "Failed to get DBus file descriptor" << std::endl;
        dbus_connection_unref(dbus_conn_);
        dbus_conn_ = nullptr;
        setOutput("󱠵", Color::DEACTIVE);
        setInterval(1); // 设置重试间隔
        return;
    }

    // 设置文件描述符，System会将其添加到epoll
    setFd(dbus_fd);
    std::cerr << "BatteryModule registered fd " << dbus_fd << " for epoll"
              << std::endl;

    // 立即更新一次
    update();
}

void BatteryModule::update() {
    try {
        // 处理DBus消息
        if (dbus_conn_) {
            dbus_connection_read_write(dbus_conn_, 0);

            // 清理所有消息
            DBusMessage *msg;
            while ((msg = dbus_connection_pop_message(dbus_conn_)) != nullptr) {
                dbus_message_unref(msg);
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
        std::string output = formatOutput(
            state, static_cast<uint64_t>(percentage), energy, energy_rate, time
        );

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

void BatteryModule::dbusGetProperty(
    const std::string &device_path, const std::string &property_name,
    int expected_type, void *value
) {
    DBusError err;
    dbus_error_init(&err);

    DBusMessage *msg = dbus_message_new_method_call(
        UPOWER_SERVICE.c_str(), device_path.c_str(),
        "org.freedesktop.DBus.Properties", "Get"
    );

    if (!msg) {
        throw std::runtime_error("Failed to create DBus message");
    }

    const char *interface = DEVICE_INTERFACE.c_str();
    const char *property = property_name.c_str();

    if (!dbus_message_append_args(
            msg, DBUS_TYPE_STRING, &interface, DBUS_TYPE_STRING, &property,
            DBUS_TYPE_INVALID
        )) {
        dbus_message_unref(msg);
        throw std::runtime_error("Failed to append DBus message args");
    }

    DBusMessage *reply =
        dbus_connection_send_with_reply_and_block(dbus_conn_, msg, -1, &err);
    dbus_message_unref(msg);

    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
        throw std::runtime_error("Failed to send DBus message");
    }

    DBusMessageIter iter, variant;
    dbus_message_iter_init(reply, &iter);
    dbus_message_iter_recurse(&iter, &variant);

    int type = dbus_message_iter_get_arg_type(&variant);
    if (type != expected_type) {
        dbus_message_unref(reply);
        throw std::runtime_error("DBus property type mismatch");
    }

    dbus_message_iter_get_basic(&variant, value);
    dbus_message_unref(reply);
}

void BatteryModule::getBatteryInfo(
    BatteryState &state, double &percentage, int64_t &time, double &energy,
    double &energy_rate
) {
    if (!dbus_conn_) {
        throw std::runtime_error("DBus connection not available");
    }

    // 获取电池状态
    uint32_t state_uint;
    dbusGetProperty(BATTERY_PATH, "State", DBUS_TYPE_UINT32, &state_uint);
    state = static_cast<BatteryState>(state_uint);

    // 获取电池信息
    dbusGetProperty(BATTERY_PATH, "Energy", DBUS_TYPE_DOUBLE, &energy);
    dbusGetProperty(BATTERY_PATH, "Percentage", DBUS_TYPE_DOUBLE, &percentage);
    dbusGetProperty(BATTERY_PATH, "EnergyRate", DBUS_TYPE_DOUBLE, &energy_rate);

    // 根据状态获取时间信息
    switch (state) {
    case BatteryState::CHARGING:
        dbusGetProperty(BATTERY_PATH, "TimeToFull", DBUS_TYPE_INT64, &time);
        break;
    case BatteryState::DISCHARGING:
        dbusGetProperty(BATTERY_PATH, "TimeToEmpty", DBUS_TYPE_INT64, &time);
        break;
    default:
        time = -1;
        break;
    }
}

std::string
BatteryModule::getBatteryIcon(BatteryState state, uint64_t percentage) {
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
    oss << "(" << hours << ":" << std::setw(2) << std::setfill('0') << minutes
        << ")";
    return oss.str();
}

std::string BatteryModule::formatOutput(
    BatteryState state, uint64_t percentage, double energy, double energy_rate,
    int64_t time
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
    if (state == BatteryState::CHARGING ||
        state == BatteryState::FULLY_CHARGED) {
        output << "<span color='" << getColorString(Color::GOOD) << "'>" << icon
               << "</span>";
    } else if (state == BatteryState::UNKNOWN ||
               state == BatteryState::PENDING_CHARGE ||
               state == BatteryState::PENDING_DISCHARGE) {
        output << "<span color='" << getColorString(Color::DEACTIVE) << "'>"
               << icon << "</span>";
    } else {
        output << "<span color='" << getColorString(color) << "'>" << icon
               << "</span>";
    }

    // 输出文字信息
    if (detailed_mode_) {
        // 详细模式：显示能量信息
        output << "\u2004<span color='" << getColorString(color) << "'>"
               << std::fixed << std::setprecision(1) << energy << "Wh</span>";
        if (time > 0) {
            output << "\u2004(" << std::fixed << std::setprecision(1)
                   << energy_rate << "W)";
        }
    } else {
        // 简单模式：显示百分比
        output << "\u2004<span color='" << getColorString(color) << "'>"
               << percentage << "%</span>";
        if (time > 0) {
            output << "\u2004" << formatTime(time);
        }
    }

    return output.str();
}
