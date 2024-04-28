#pragma once

#include <vector>

#include "class.hpp"

namespace ts {

class StructClass : public Class {
    std::vector<Class::Ptr> fields_;

public:
    using Ptr = util::Ptr<StructClass>;

    StructClass(std::string name) : Class(std::move(name)) {
    }

    // Maybe should not be public
    auto AddField(const Class::Ptr& field) -> void {
        fields_.push_back(field);
    }

    [[nodiscard]] auto Serialize() const -> std::string override {
        auto result = "_struct@" + name_ + "_<";
        for (auto& field : fields_) {
            result.append(field->Serialize());
        }
        result.append(">_");
        return result;
    }

    [[nodiscard]] auto Size() const -> std::optional<size_t> override {
        auto result = 0;
        for (auto& field : fields_) {
            auto size = field->Size();
            if (size.has_value()) {
                result += size.value();
            } else {
                return std::nullopt;
            }
        }
        return result;
    }

    [[nodiscard]] auto GetFields() const noexcept -> const std::vector<Class::Ptr>& {
        return fields_;
    }

    [[nodiscard]] auto Count() const -> size_t override {
        size_t count = 0;
        for (auto& field : fields_) {
            count += field->Count();
        }
        return count;
    }
};
}  // namespace ts