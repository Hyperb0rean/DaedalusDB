#pragma once

#include <memory>
#include <optional>
#include <string>

#include "object.hpp"
#include "relation_class.hpp"

namespace ts {

class Relation : public ts::Object {
    Id ingress_id_;
    Id egress_id_;
    std::optional<Object::Ptr> attributes_object_ = std::nullopt;

public:
    using Ptr = util::Ptr<Relation>;
    Relation(const Class::Ptr& argclass, Id ingress_id, Id egress_id)
        : ingress_id_(ingress_id), egress_id_(egress_id) {
        this->class_ = argclass;
        if (util::As<RelationClass>(argclass)->AttributesClass().has_value()) {
            error::TypeError("Relation has attributes, but no provided");
        }
    }
    Relation(const Class::Ptr& argclass, Id ingress_id, Id egress_id,
             const Object::Ptr& arguments_object)
        : Relation(argclass, ingress_id, egress_id) {
        if (!util::As<RelationClass>(argclass)->AttributesClass().has_value()) {
            error::TypeError("Relation has no attributes, but any provided");
        }
        attributes_object_ = arguments_object;
    }

    virtual ~Relation() = default;

    [[nodiscard]] size_t Size() const override {
        return 2 * sizeof(Id) +
               (attributes_object_.has_value() ? attributes_object_.value()->Size() : 0);
    }
    mem::Offset Write(mem::File::Ptr& file, mem::Offset offset) const override {
        file->Write(ingress_id_, offset);
        offset += sizeof(Id);
        file->Write(egress_id_, offset);
        offset += sizeof(Id);
        if (attributes_object_.has_value()) {
            return attributes_object_.value()->Write(file, offset);
        } else {
            return offset;
        }
    }
    void Read(mem::File::Ptr& file, mem::Offset offset) override {
        ingress_id_ = file->Read<Id>(offset);
        offset += sizeof(Id);
        egress_id_ = file->Read<Id>(offset);
        offset += sizeof(Id);
        if (attributes_object_.has_value()) {
            attributes_object_.value()->Read(file, offset);
        }
    }
    [[nodiscard]] std::string ToString() const override {
        return std::string("relation: ")
            .append(class_->Name())
            .append(" ( ingress: ")
            .append("( id: ")
            .append(std::to_string(ingress_id_))
            .append(" , class: ")
            .append(util::As<RelationClass>(class_)->IngressClass()->Name())
            .append(" ), ")
            .append("egress: ")
            .append("( id: ")
            .append(std::to_string(egress_id_))
            .append(" , class: ")
            .append(util::As<RelationClass>(class_)->EgressClass()->Name())
            .append(" ), ")
            .append(attributes_object_.has_value()
                        ? std::string("attributes: ").append(attributes_object_.value()->ToString())
                        : "")
            .append(" ) ");
    }

    [[nodiscard]] Id IngressId() const {
        return ingress_id_;
    }
    [[nodiscard]] Id EgressId() const {
        return egress_id_;
    }
    [[nodiscard]] std::optional<Object::Ptr> Attributes() const {
        return attributes_object_;
    }
};
}  // namespace ts