#pragma once
#include <chrono>
#include <cstring>
#include <ctime>
#include <memory>
#include <string>

#include "error.hpp"

namespace util {

template <typename T, typename Base>
inline bool Is(const std::shared_ptr<Base>& obj) {
    return dynamic_cast<T*>(obj.get()) != nullptr;
}

template <typename T, typename Base>
inline std::shared_ptr<T> As(const std::shared_ptr<Base>& obj) {
    if (Is<T>(obj)) {
        return std::dynamic_pointer_cast<T>(obj);
    } else {
        throw error::BadCast("");
    }
}

}  // namespace util