#pragma once
#include <iostream>
#include <string>

namespace util {
class Logger {
public:
    Logger() = default;
    virtual ~Logger(){};
    virtual void Log(std::string_view) = 0;
    virtual void Error(std::string_view) = 0;
    virtual void Verbose(std::string_view) = 0;
};

class ConsoleLogger : public Logger {
    std::ostream& cout_;
    std::ostream& cerr_;

public:
    ~ConsoleLogger(){};

    ConsoleLogger(std::ostream& cout = std::cout, std::ostream& cerr = std::cerr)
        : cout_(cout), cerr_(cerr) {
    }

    void Log(std::string_view message) override {
        cout_ << GetCurrentTime() << " | LOG | " << message << std::endl;
    }
    void Error(std::string_view message) override {
        cout_ << GetCurrentTime() << " | ERR | " << message << std::endl;
    }
    void Verbose(std::string_view message) override {
    }
};

class VerboseConsoleLogger : public Logger {
    std::ostream& cout_;
    std::ostream& cerr_;

public:
    ~VerboseConsoleLogger(){};

    VerboseConsoleLogger(std::ostream& cout = std::cout, std::ostream& cerr = std::cerr)
        : cout_(cout), cerr_(cerr) {
    }

    void Log(std::string_view message) override {
        cout_ << GetCurrentTime() << " | LOG | " << message << std::endl;
    }
    void Error(std::string_view message) override {
        cout_ << GetCurrentTime() << " | ERR | " << message << std::endl;
    }
    void Verbose(std::string_view message) override {
        cout_ << GetCurrentTime() << " | LOG | " << message << std::endl;
    }
};

class EmptyLogger : public Logger {

public:
    ~EmptyLogger(){};

    EmptyLogger() {
    }

    void Log(std::string_view) override {
    }
    void Error(std::string_view) override {
    }
    void Verbose(std::string_view) override {
    }
};
}  // namespace util