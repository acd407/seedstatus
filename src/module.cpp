#include <module.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

// 颜色字符串映射
static constexpr const char *colorStrings[] = {
    "#6A6862", // DEACTIVE
    "#729FCF", // COOL
    "#98BC37", // GOOD
    "#FCE8C3", // IDLE
    "#FED06E", // WARNING
    "#F75341"  // CRITICAL
};

// 获取颜色的字符串表示
std::string getColorString(Color color) {
    const int index = static_cast<int>(color);
    constexpr int colorCount = sizeof(colorStrings) / sizeof(colorStrings[0]);

    if (index >= 0 && index < colorCount) {
        return colorStrings[index];
    }
    return colorStrings[static_cast<int>(Color::IDLE)]; // 默认返回IDLE颜色
}

// Module类实现
Module::Module(const std::string &name) : name_(name) {
    updateLastUpdateTime();
}

std::string Module::getName() const {
    return name_;
}

std::string Module::getOutput() const {
    return output_;
}

void Module::setOutput(const std::string &output, Color color) {
    output_ = output;
    color_ = getColorString(color);
    updateLastUpdateTime();
}

void Module::setInterval(uint64_t interval) {
    interval_ = interval;
}

uint64_t Module::getInterval() const {
    return interval_;
}

void Module::setState(uint64_t state) {
    state_ = state;
}

uint64_t Module::getState() const {
    return state_;
}

void Module::handleClick(uint64_t button) {
    (void)button;
    // 默认实现不做任何事情
}

void Module::init() {
    // 默认实现不做任何事情
}

void Module::setFd(int fd) {
    fd_ = fd;
}

int Module::getFd() const {
    return fd_;
}

bool Module::shouldDelete() const {
    return should_delete_;
}

void Module::markForDeletion() {
    should_delete_ = true;
}

std::string Module::toJson() const {
    try {
        json j;
        j["name"] = name_;
        j["separator"] = false;
        j["separator_block_width"] = 0;
        j["markup"] = "pango";
        j["full_text"] = output_;
        j["color"] = color_;
        return j.dump();
    } catch (const std::exception &e) {
        throw std::runtime_error(
            "Failed to create JSON string: " + std::string(e.what())
        );
    }
}

std::chrono::steady_clock::time_point Module::getLastUpdateTime() const {
    return last_update_time_;
}

bool Module::needsUpdate() const {
    if (interval_ == 0) {
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - last_update_time_
    );
    return elapsed.count() >= static_cast<int64_t>(interval_);
}

void Module::updateLastUpdateTime() {
    last_update_time_ = std::chrono::steady_clock::now();
}

// 读取Uint64格式的文件内容
uint64_t Module::readUint64File(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open " + path);
    }

    uint64_t value = 0;
    file >> value;

    if (!file) {
        throw std::runtime_error("Failed to read from " + path);
    }

    return value;
}

// ModuleManager类实现
void ModuleManager::addModule(std::shared_ptr<Module> module) {
    if (!module) {
        throw std::invalid_argument("Module cannot be null");
    }

    modules_.push_back(module);
}

size_t ModuleManager::getModuleCount() const {
    return modules_.size();
}

std::shared_ptr<Module> ModuleManager::getModule(size_t index) const {
    if (index >= modules_.size()) {
        throw std::out_of_range("Module index out of range");
    }
    return modules_[index];
}

std::shared_ptr<Module>
ModuleManager::getModuleByName(const std::string &name) const {
    for (const auto &module : modules_) {
        if (module->getName() == name) {
            return module;
        }
    }
    return nullptr;
}

void ModuleManager::removeMarkedModules() {
    auto it = std::remove_if(
        modules_.begin(), modules_.end(),
        [](const std::shared_ptr<Module> &module) {
            return module->shouldDelete();
        }
    );
    modules_.erase(it, modules_.end());
}

const std::vector<std::shared_ptr<Module>> &ModuleManager::getModules() const {
    return modules_;
}

void ModuleManager::outputModules() const {
    try {
        std::cout << '[';
        bool first = true;

        for (const auto &module : modules_) {
            if (!module)
                continue;

            std::string output;
            try {
                output = module->getOutput();
            } catch (const std::exception &e) {
                std::cerr << "Error getting output from module "
                          << module->getName() << ": " << e.what() << std::endl;
                continue;
            }

            if (!output.empty()) {
                if (!first) {
                    std::cout << ',';
                }

                try {
                    std::cout << module->toJson();

                    // 添加空格分隔符
                    std::cout
                        << ",{\"full_text\":\" "
                           "\",\"separator\":false,\"separator_block_width\":0,"
                           "\"markup\":\"pango\"}";
                    first = false;
                } catch (const std::exception &e) {
                    std::cerr << "Error generating JSON for module "
                              << module->getName() << ": " << e.what()
                              << std::endl;
                }
            }
        }

        std::cout << "]," << std::endl;
        std::cout.flush();
    } catch (const std::exception &e) {
        std::cerr << "Error in outputModules: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error in outputModules" << std::endl;
    }
}