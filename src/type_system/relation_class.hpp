#pragma once

#include <optional>

#include "class.hpp"

namespace ts {

// Should be twice size of mem::PageOffset;
using Id = uint64_t;

class RelationClass : public ts::Class {
private:
    ts::Class::Ptr ingress_class_;
    ts::Class::Ptr egress_class_;
    std::optional<ts::Class::Ptr> attributes_class_ = std::nullopt;

public:
    using Ptr = util::Ptr<Class>;
    RelationClass(std::string name, ts::Class::Ptr ingress_class, ts::Class::Ptr egress_class)
        : ts::Class(std::move(name)), ingress_class_(ingress_class), egress_class_(egress_class) {
    }

    RelationClass(std::string name, ts::Class::Ptr ingress_class, ts::Class::Ptr egress_class,
                  ts::Class::Ptr attributes_class)
        : RelationClass(std::move(name), ingress_class, egress_class) {
        attributes_class_ = attributes_class;
    }
    ~RelationClass() = default;

    [[nodiscard]] std::string Serialize() const override {
        return std::string("_relation@")
            .append(name_)
            .append("_")
            .append(ingress_class_->Serialize())
            .append(egress_class_->Serialize())
            .append(attributes_class_.has_value() ? "1" : "_")
            .append(attributes_class_.has_value() ? attributes_class_.value()->Serialize() : "");
    }

    [[nodiscard]] std::optional<size_t> Size() const override {
        if (!attributes_class_.has_value()) {
            return 2 * sizeof(Id);
        }
        if (attributes_class_.value()->Size().has_value()) {
            return attributes_class_.value()->Size().value() + 2 * sizeof(Id);
        } else {
            return std::nullopt;
        }
    }

    [[nodiscard]] std::string Name() const override {
        return name_;
    }
    [[nodiscard]] size_t Count() const override {
        // Should think about this more
        if (attributes_class_.has_value()) {
            return attributes_class_.value()->Count() + 2;
        } else {
            return 2;
        }
    }

    [[nodiscard]] Class::Ptr IngressClass() const {
        return ingress_class_;
    }
    [[nodiscard]] Class::Ptr EgressClass() const {
        return egress_class_;
    }
    [[nodiscard]] std::optional<Class::Ptr> AttributesClass() const {
        return attributes_class_;
    }
};
}  // namespace ts