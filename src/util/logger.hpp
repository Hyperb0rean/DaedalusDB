#pragma once

#include <any>
#include <iostream>
#include <string>

#include "utils.hpp"

#define LOGGER logger_
#define DECLARE_LOGGER ::util::Ptr<::util::Logger> LOGGER

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
    virtual void LogImpl(LogLevel, PrintableAny&&) const = 0;
    virtual void LogHeader(LogLevel) const = 0;
    virtual void LogFooter(LogLevel) const = 0;

public:
    Logger() = default;

    template <typename... Args>
    void Log(LogLevel level, Args&&... args) const {
        LogHeader(level);
        (LogImpl(level, PrintableAny(std::forward<Args>(args))), ...);
        LogFooter(level);
    }

    virtual ~Logger(){};
};

#define DEFINE_LOGGER(logger) ::util::Ptr<::util::Logger> logger

class DebugLogger : public Logger {
protected:
    std::ostream& cout_;
    std::ostream& cerr_;

public:
    ~DebugLogger(){};

    DebugLogger(std::ostream& cout = std::cout, std::ostream& cerr = std::cerr)
        : cout_(cout), cerr_(cerr) {
    }

    void LogFooter([[maybe_unused]] LogLevel level) const override {
        cout_ << std::endl;
    }

    void LogHeader(LogLevel level) const override {
        cout_ << GetCurrentTime() << " | " << LevelToString(level) << " | ";
    }

    void LogImpl([[maybe_unused]] LogLevel level, PrintableAny&& data) const override {
        cout_ << data;
    }
};

#define DEBUG_LOGGER util::MakePtr<::util::DebugLogger>()

class ConsoleLogger : public DebugLogger {
public:
    ConsoleLogger(std::ostream& cout = std::cout, std::ostream& cerr = std::cerr)
        : DebugLogger(cout, cerr) {
    }

    void LogFooter(LogLevel level) const override {
        if (level != LogLevel::kDebug) {
            cout_ << std::endl;
        }
    }

    void LogHeader(LogLevel level) const override {
        if (level != LogLevel::kDebug) {
            cout_ << GetCurrentTime() << " | " << LevelToString(level) << " | ";
        }
    }

    void LogImpl(LogLevel level, PrintableAny&& data) const override {
        if (level != LogLevel::kDebug) {
            cout_ << data;
        }
    }
};

#define CONSOLE_LOGGER util::MakePtr<::util::ConsoleLogger>()

class EmptyLogger : public Logger {

public:
    ~EmptyLogger(){};

    EmptyLogger() {
    }

    void LogFooter([[maybe_unused]] LogLevel level) const override {
    }

    void LogHeader([[maybe_unused]] LogLevel level) const override {
    }

    void LogImpl([[maybe_unused]] LogLevel level,
                 [[maybe_unused]] PrintableAny&& data) const override {
    }
};

#define EMPTY_LOGGER util::MakePtr<::util::EmptyLogger>()

#define DEFAULT_LOGGER(logger) DEFINE_LOGGER(logger) = EMPTY_LOGGER
}  // namespace util