#pragma once
#include <cerrno>
#include <stdexcept>

namespace utils {
inline const std::string GetCurrentTime() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return buf;
}
}  // namespace utils

namespace error {

class Error : public std::exception {
private:
    std::string desc_;

public:
    explicit Error(std::string desc) : desc_{utils::GetCurrentTime() + " | " + desc} {
        perror(desc_.c_str());
        std::abort();
    }
};

class IoError : public Error {
public:
    explicit IoError(std::string desc) : Error{"IoError | " + desc} {};
};

class BadArgument : public Error {
public:
    explicit BadArgument(std::string desc) : Error{"BadArgument | " + desc} {};
};

class BadCast : public Error {
public:
    explicit BadCast(std::string desc) : Error{"BadCast | " + desc} {};
};

class NotImplemented : public Error {
public:
    explicit NotImplemented(std::string desc) : Error{"NotImplemented | " + desc} {};
};

class StructureError : public Error {
public:
    explicit StructureError(std::string desc) : Error{"StructureError | " + desc} {};
};

}  // namespace error
