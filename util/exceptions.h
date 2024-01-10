#pragma once
#include <cerrno>
#include <stdexcept>

#include "utils.h"

namespace error {

class Error : public std::runtime_error {
public:
    explicit Error(std::string desc)
        : std::runtime_error{GetCurrentTime() + "| " + desc + ": " + std::strerror(errno)} {
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
}  // namespace error