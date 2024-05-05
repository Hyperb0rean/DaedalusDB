#pragma once

#include <cctype>

#include "class.hpp"

namespace ts {
template <typename T>
requires std::is_arithmetic_v<T>
class PrimitiveClass : public Class {
public:
    using Ptr = util::Ptr<PrimitiveClass<T>>;

    explicit PrimitiveClass(std::string name) : Class(std::move(name)) {
    }
    [[nodiscard]] auto Serialize() const -> std::string override {
        std::string result = "_";
        result.append(TypeName<T>());
        result.append("@").append(name_).append("_");
        result.erase(std::remove_if(result.begin(), result.end(), isspace), result.end());
        return result;
    }
    [[nodiscard]] auto Size() const -> std::optional<size_t> override {
        return sizeof(T);
    }

    [[nodiscard]] auto Count() const -> size_t override {
        return 1;
    }
};
}  // namespace ts