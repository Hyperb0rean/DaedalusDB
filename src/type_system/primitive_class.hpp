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
    [[nodiscard]] std::string Serialize() const override {
        std::string result = "_";
        result += TypeName<T>();
        result.append("@").append(name_).append("_");
        result.erase(std::remove_if(result.begin(), result.end(), isspace), result.end());
        return result;
    }
    [[nodiscard]] std::optional<size_t> Size() const override {
        return sizeof(T);
    }

    [[nodiscard]] size_t Count() const override {
        return 1;
    }
};
}  // namespace ts