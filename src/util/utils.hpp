#pragma once
#include <any>
#include <memory>
#include <string>

#include "error.hpp"

namespace util {

class PrintableAny {

    std::any value_;

    using PrintFunctor = void (*)(std::ostream&, const std::any&);
    PrintFunctor functor_;

    friend auto operator<<(std::ostream& os, const PrintableAny& any) -> std::ostream& {
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

template <typename T>
using Ptr = ::std::shared_ptr<T>;

template <typename T, typename... Args>
[[nodiscard]] inline auto MakePtr(Args&&... args) -> Ptr<T> {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename Base>
[[nodiscard]] inline auto Is(const util::Ptr<Base>& obj) -> bool {
    return dynamic_cast<T*>(obj.get()) != nullptr;
}

template <typename T, typename Base>
[[nodiscard]] inline auto As(const util::Ptr<Base>& obj) -> Ptr<T> {
    if (Is<T>(obj)) {
        return std::dynamic_pointer_cast<T>(obj);
    } else {
        throw error::BadCast("");
    }
}

}  // namespace util