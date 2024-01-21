#pragma once
#include <iostream>
#include <string>

namespace util {
class Logger {
public:
    Logger() = default;
    virtual ~Logger(){};
    virtual void Debug(std::string_view) = 0;
    virtual void Info(std::string_view) = 0;
    virtual void Warn(std::string_view) = 0;
    virtual void Error(std::string_view) = 0;
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

    void Debug(std::string_view message) override {
    }
    void Info(std::string_view message) override {
        cout_ << GetCurrentTime() << " | INFO  | " << message << std::endl;
    }
    void Warn(std::string_view message) override {
        cout_ << GetCurrentTime() << " | WARN  | " << message << std::endl;
    }
    void Error(std::string_view message) override {
        cerr_ << GetCurrentTime() << " | ERROR | " << message << std::endl;
    }
};

class DebugLogger : public ConsoleLogger {
public:
    DebugLogger(std::ostream& cout = std::cout, std::ostream& cerr = std::cerr)
        : ConsoleLogger(cout, cerr) {
    }
    void Debug(std::string_view message) override {
        this->cout_ << GetCurrentTime() << " | DEBUG | " << message << std::endl;
    }
};

class EmptyLogger : public Logger {

public:
    ~EmptyLogger(){};

    EmptyLogger() {
    }

    void Debug(std::string_view) override {
    }
    void Info(std::string_view) override {
    }
    void Warn(std::string_view) override {
    }
    void Error(std::string_view) override {
    }
};
}  // namespace util