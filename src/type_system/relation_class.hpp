#pragma once

#include "class.hpp"

namespace ts {

// Should be twice size of mem::PageOffset;
using Id = uint64_t;

class RelationClass : public ts::Class {
private:
    ts::Class::Ptr ingress_class_;
    ts::Class::Ptr egress_class_;
    std::optional<ts::Class::Ptr> atributes_class_;

public:
    using Ptr = util::Ptr<Class>;

    explicit RelationClass(std::string name, ts::Class::Ptr ingress_class,
                           ts::Class::Ptr egress_class,
                           std::optional<ts::Class::Ptr> atributes_class = std::nullopt)
        : ts::Class(std::move(name)),
          ingress_class_(ingress_class),
          egress_class_(egress_class),
          atributes_class_(atributes_class) {
    }
    ~RelationClass() = default;

    [[nodiscard]] std::string Serialize() const override {
        return std::string("_relation@")
            .append(name_)
            .append("_")
            .append(ingress_class_->Serialize())
            .append(egress_class_->Serialize())
            .append(atributes_class_.has_value() ? "1" : "_")
            .append(atributes_class_.has_value() ? atributes_class_.value()->Serialize() : "");
    }

    [[nodiscard]] std::optional<size_t> Size() const override {
        if (!atributes_class_.has_value()) {
            return 2 * sizeof(Id);
        }
        if (atributes_class_.value()->Size().has_value()) {
            return atributes_class_.value()->Size().value() + 2 * sizeof(Id);
        } else {
            return std::nullopt;
        }
    }

    [[nodiscard]] std::string Name() const override {
        return name_;
    }
    [[nodiscard]] size_t Count() const override {
        // Should think about this more
        if (atributes_class_.has_value()) {
            return atributes_class_.value()->Count() + 2;
        } else {
            return 2;
        }
    }
};
}  // namespace ts