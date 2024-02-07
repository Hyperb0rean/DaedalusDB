#pragma once
#include <chrono>
#include <cstring>
#include <ctime>
#include <memory>
#include <string>

#include "error.hpp"

namespace util {

class PrintableAny {

    std::any value_;

    using PrintFunctor = void (*)(std::ostream&, const std::any&);
    PrintFunctor functor_;

    friend std::ostream& operator<<(std::ostream& os, const PrintableAny& any) {
        any.functor_(os, any.value_);
        return os;
    }

public:
    template <typename T>
    PrintableAny(T&& value) : value_(std::forward<T>(value)) {
        functor_ = [](std::ostream& os, const std::any& val) {
            os << std::any_cast<std::decay_t<T>>(val);
        };
    }
};

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