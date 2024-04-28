#pragma once

#include <optional>

#include "class.hpp"

namespace ts {

// Should be twice size of mem::PageOffset;
using Id = uint64_t;

class RelationClass : public ts::Class {
private:
    ts::Class::Ptr from_class_;
    ts::Class::Ptr to_class_;
    std::optional<ts::Class::Ptr> attributes_class_ = std::nullopt;

public:
    using Ptr = util::Ptr<RelationClass>;
    RelationClass(std::string name, ts::Class::Ptr from_class, ts::Class::Ptr to_class)
        : ts::Class(std::move(name)), from_class_(from_class), to_class_(to_class) {
    }

    RelationClass(std::string name, ts::Class::Ptr from_class, ts::Class::Ptr to_class,
                  ts::Class::Ptr attributes_class)
        : RelationClass(std::move(name), from_class, to_class) {
        attributes_class_ = attributes_class;
    }
    ~RelationClass() = default;

    [[nodiscard]] auto Serialize() const -> std::string override {
        return std::string{"_relation@"}
            .append(name_)
            .append("_")
            .append(from_class_->Serialize())
            .append(to_class_->Serialize())
            .append(attributes_class_.has_value() ? "1" : "_")
            .append(attributes_class_.has_value() ? attributes_class_.value()->Serialize() : "");
    }

    [[nodiscard]] auto Size() const -> std::optional<size_t> override {
        if (!attributes_class_.has_value()) {
            return 2 * sizeof(Id);
        }
        if (attributes_class_.value()->Size().has_value()) {
            return attributes_class_.value()->Size().value() + 2 * sizeof(Id);
        } else {
            return std::nullopt;
        }
    }
    [[nodiscard]] auto Count() const -> size_t override {
        // Should think about this more
        if (attributes_class_.has_value()) {
            return attributes_class_.value()->Count() + 2;
        } else {
            return 2;
        }
    }

    [[nodiscard]] auto FromClass() const noexcept -> Class::Ptr {
        return from_class_;
    }
    [[nodiscard]] auto ToClass() const noexcept -> Class::Ptr {
        return to_class_;
    }
    [[nodiscard]] auto AttributesClass() const noexcept -> std::optional<Class::Ptr> {
        return attributes_class_;
    }
};
}  // namespace ts