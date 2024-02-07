#pragma once

#include <any>
#include <iostream>
#include <string>

#include "utils.hpp"

#define LOGGER logger_
#define DECLARE_LOGGER std::shared_ptr<::util::Logger> LOGGER

#define DEBUG(...) LOGGER->Log(::util::LogType::kDebug, __VA_ARGS__)
#define INFO(...) LOGGER->Log(::util::LogType::kInfo, __VA_ARGS__)
#define WARN(...) LOGGER->Log(::util::LogType::kWarn, __VA_ARGS__)
#define ERROR(...) LOGGER->Log(::util::LogType::kError, __VA_ARGS__)

namespace util {

enum class LogType {
    kDebug,
    kInfo,
    kWarn,
    kError,
};

inline std::string_view ModeToString(LogType mode) noexcept {
    switch (mode) {
        case LogType::kDebug:
            return "D";
        case LogType::kInfo:
            return "I";
        case LogType::kWarn:
            return "W";
        case LogType::kError:
            return "E";
    }
    return "";
}

class Logger {
protected:
    virtual void LogImpl(LogType, PrintableAny&&) = 0;
    virtual void LogHeader(LogType) = 0;
    virtual void LogFooter(LogType) = 0;

    template <typename Arg>
    void IteratePack(LogType mode, Arg&& arg) {
        LogImpl(mode, PrintableAny{std::forward<Arg>(arg)});
    }

    template <typename Arg, typename... Args>
    void IteratePack(LogType mode, Arg&& arg, Args&&... args) {
        LogImpl(mode, PrintableAny{std::forward<Arg>(arg)});
        IteratePack(mode, std::forward<Args>(args)...);
    }

public:
    Logger() = default;

    template <typename... Args>
    void Log(LogType mode, Args&&... args) {
        LogHeader(mode);
        IteratePack(mode, std::forward<Args>(args)...);
        LogFooter(mode);
    }

    virtual ~Logger(){};
};

class ConsoleLogger : public Logger {
protected:
    std::ostream& cout_;
    std::ostream& cerr_;

public:
    ~ConsoleLogger(){};

    ConsoleLogger(std::ostream& cout = std::cout, std::ostream& cerr = std::cerr)
        : cout_(cout), cerr_(cerr) {
    }

    void LogFooter(LogType mode) override {
        cout_ << std::endl;
    }

    void LogHeader(LogType mode) override {
        cout_ << GetCurrentTime() << " | " << ModeToString(mode) << " | ";
    }

    void LogImpl(LogType mode, PrintableAny&& data) override {
        cout_ << data;
    }
};

class SimpleLogger : public ConsoleLogger {
public:
    SimpleLogger(std::ostream& cout = std::cout, std::ostream& cerr = std::cerr)
        : ConsoleLogger(cout, cerr) {
    }

    void LogFooter(LogType mode) override {
        if (mode != LogType::kDebug) {
            cout_ << std::endl;
        }
    }

    void LogHeader(LogType mode) override {
        if (mode != LogType::kDebug) {
            cout_ << GetCurrentTime() << " | " << ModeToString(mode) << " | ";
        }
    }

    void LogImpl(LogType mode, PrintableAny&& data) override {
        if (mode != LogType::kDebug) {
            cout_ << data;
        }
    }
};

class EmptyLogger : public Logger {

public:
    ~EmptyLogger(){};

    EmptyLogger() {
    }

    void LogFooter(LogType mode) override {
    }

    void LogHeader(LogType mode) override {
    }

    void LogImpl(LogType mode, PrintableAny&& data) override {
    }
};
}  // namespace util