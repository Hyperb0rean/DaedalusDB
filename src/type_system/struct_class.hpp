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

    void AddField(const Class::Ptr& field) {
        fields_.push_back(field);
    }

    [[nodiscard]] std::string Serialize() const override {
        auto result = "_struct@" + name_ + "_<";
        for (auto& field : fields_) {
            result.append(field->Serialize());
        }
        result.append(">_");
        return result;
    }

    [[nodiscard]] std::optional<size_t> Size() const override {
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

    [[nodiscard]] const std::vector<Class::Ptr>& GetFields() const {
        return fields_;
    }

    [[nodiscard]] size_t Count() const override {
        size_t count = 0;
        for (auto& field : fields_) {
            count += field->Count();
        }
        return count;
    }
};
}  // namespace ts