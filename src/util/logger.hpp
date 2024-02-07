#pragma once

#include <any>
#include <iostream>
#include <string>

#include "utils.hpp"

#define LOGGER logger_
#define DECLARE_LOGGER std::shared_ptr<::util::Logger> LOGGER

#define DEBUG(...) LOGGER->Log(::util::LogLevel::kDebug, __VA_ARGS__)
#define INFO(...) LOGGER->Log(::util::LogLevel::kInfo, __VA_ARGS__)
#define WARN(...) LOGGER->Log(::util::LogLevel::kWarn, __VA_ARGS__)
#define ERROR(...) LOGGER->Log(::util::LogLevel::kError, __VA_ARGS__)

namespace util {

enum class LogLevel {
    kDebug,
    kInfo,
    kWarn,
    kError,
};

inline std::string_view LevelToString(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::kDebug:
            return "D";
        case LogLevel::kInfo:
            return "I";
        case LogLevel::kWarn:
            return "W";
        case LogLevel::kError:
            return "E";
    }
    return "";
}

class Logger {
protected:
    virtual void LogImpl(LogLevel, PrintableAny&&) = 0;
    virtual void LogHeader(LogLevel) = 0;
    virtual void LogFooter(LogLevel) = 0;

public:
    Logger() = default;

    template <typename... Args>
    void Log(LogLevel level, Args&&... args) {
        LogHeader(level);
        (LogImpl(level, PrintableAny(std::forward<Args>(args))), ...);
        LogFooter(level);
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

    void LogFooter(LogLevel level) override {
        cout_ << std::endl;
    }

    void LogHeader(LogLevel level) override {
        cout_ << GetCurrentTime() << " | " << LevelToString(level) << " | ";
    }

    void LogImpl(LogLevel level, PrintableAny&& data) override {
        cout_ << data;
    }
};

class SimpleLogger : public ConsoleLogger {
public:
    SimpleLogger(std::ostream& cout = std::cout, std::ostream& cerr = std::cerr)
        : ConsoleLogger(cout, cerr) {
    }

    void LogFooter(LogLevel level) override {
        if (level != LogLevel::kDebug) {
            cout_ << std::endl;
        }
    }

    void LogHeader(LogLevel level) override {
        if (level != LogLevel::kDebug) {
            cout_ << GetCurrentTime() << " | " << LevelToString(level) << " | ";
        }
    }

    void LogImpl(LogLevel level, PrintableAny&& data) override {
        if (level != LogLevel::kDebug) {
            cout_ << data;
        }
    }
};

class EmptyLogger : public Logger {

public:
    ~EmptyLogger(){};

    EmptyLogger() {
    }

    void LogFooter(LogLevel level) override {
    }

    void LogHeader(LogLevel level) override {
    }

    void LogImpl(LogLevel level, PrintableAny&& data) override {
    }
};
}  // namespace util