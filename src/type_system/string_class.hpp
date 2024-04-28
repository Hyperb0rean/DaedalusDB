#pragma once

#include "class.hpp"

namespace ts {
class StringClass : public Class {
public:
    using Ptr = util::Ptr<StringClass>;

    explicit StringClass(std::string name) : Class(std::move(name)) {
    }
    [[nodiscard]] auto Serialize() const -> std::string override {
        return "_string@" + name_ + "_";
    }
    [[nodiscard]] auto Size() const -> std::optional<size_t> override {
        return std::nullopt;
    }

    [[nodiscard]] auto Count() const -> size_t override {
        return 1;
    }
};
}  // namespace ts