#pragma once

#include "class.hpp"

namespace ts {
class StringClass : public Class {
public:
    using Ptr = util::Ptr<StringClass>;

    explicit StringClass(std::string name) : Class(std::move(name)) {
    }
    [[nodiscard]] std::string Serialize() const override {
        return "_string@" + name_ + "_";
    }
    [[nodiscard]] std::optional<size_t> Size() const override {
        return std::nullopt;
    }

    [[nodiscard]] size_t Count() const override {
        return 1;
    }
};
}  // namespace ts