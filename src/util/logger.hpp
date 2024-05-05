#pragma once

#include <iostream>

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

inline auto LevelToString(LogLevel level) noexcept -> std::string_view {
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
    virtual auto LogImpl(LogLevel, PrintableAny&&) const -> void = 0;
    virtual auto LogHeader(LogLevel) const -> void = 0;
    virtual auto LogFooter(LogLevel) const -> void = 0;

public:
    Logger() = default;

    template <typename... Args>
    auto Log(LogLevel level, Args&&... args) const -> void {
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

    auto LogFooter([[maybe_unused]] LogLevel level) const -> void override {
        cout_ << std::endl;
    }

    auto LogHeader(LogLevel level) const -> void override {
        cout_ << GetCurrentTime() << " | " << LevelToString(level) << " | ";
    }

    auto LogImpl([[maybe_unused]] LogLevel level, PrintableAny&& data) const -> void override {
        cout_ << data;
    }
};

#define DEBUG_LOGGER util::MakePtr<::util::DebugLogger>()

class ConsoleLogger : public DebugLogger {
public:
    ConsoleLogger(std::ostream& cout = std::cout, std::ostream& cerr = std::cerr)
        : DebugLogger(cout, cerr) {
    }

    auto LogFooter(LogLevel level) const -> void override {
        if (level != LogLevel::kDebug) {
            cout_ << std::endl;
        }
    }

    auto LogHeader(LogLevel level) const -> void override {
        if (level != LogLevel::kDebug) {
            cout_ << GetCurrentTime() << " | " << LevelToString(level) << " | ";
        }
    }

    auto LogImpl(LogLevel level, PrintableAny&& data) const -> void override {
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

    auto LogFooter([[maybe_unused]] LogLevel level) const -> void override {
    }

    auto LogHeader([[maybe_unused]] LogLevel level) const -> void override {
    }

    auto LogImpl([[maybe_unused]] LogLevel level, [[maybe_unused]] PrintableAny&& data) const
        -> void override {
    }
};

#define EMPTY_LOGGER util::MakePtr<::util::EmptyLogger>()

#define DEFAULT_LOGGER(logger) DEFINE_LOGGER(logger) = EMPTY_LOGGER
}  // namespace util