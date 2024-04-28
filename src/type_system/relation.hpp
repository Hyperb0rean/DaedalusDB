#pragma once

#include <memory>
#include <optional>
#include <string>

#include "object.hpp"
#include "relation_class.hpp"

namespace ts {

class Relation : public ts::Object {
    Id from_id_;
    Id to_id_;
    std::optional<Object::Ptr> attributes_object_ = std::nullopt;

public:
    using Ptr = util::Ptr<Relation>;
    Relation(const Class::Ptr& argclass, Id from_id, Id to_id) : from_id_(from_id), to_id_(to_id) {
        this->class_ = argclass;
        if (util::As<RelationClass>(argclass)->AttributesClass().has_value()) {
            error::TypeError("Relation has attributes, but no provided");
        }
    }
    Relation(const Class::Ptr& argclass, Id from_id, Id to_id, const Object::Ptr& arguments_object)
        : Relation(argclass, from_id, to_id) {
        if (!util::As<RelationClass>(argclass)->AttributesClass().has_value()) {
            error::TypeError("Relation has no attributes, but any provided");
        }
        attributes_object_ = arguments_object;
    }

    virtual ~Relation() = default;

    [[nodiscard]] auto Size() const -> size_t override {
        return 2 * sizeof(Id) +
               (attributes_object_.has_value() ? attributes_object_.value()->Size() : 0);
    }
    auto Write(mem::File::Ptr& file, mem::Offset offset) const -> mem::Offset override {
        file->Write(from_id_, offset);
        offset += sizeof(Id);
        file->Write(to_id_, offset);
        offset += sizeof(Id);
        if (attributes_object_.has_value()) {
            return attributes_object_.value()->Write(file, offset);
        } else {
            return offset;
        }
    }
    auto Read(mem::File::Ptr& file, mem::Offset offset) -> void override {
        from_id_ = file->Read<Id>(offset);
        offset += sizeof(Id);
        to_id_ = file->Read<Id>(offset);
        offset += sizeof(Id);
        if (attributes_object_.has_value()) {
            attributes_object_.value()->Read(file, offset);
        }
    }
    [[nodiscard]] auto ToString() const -> std::string override {
        return std::string("relation: ")
            .append(class_->Name())
            .append(" ( from: ")
            .append("( id: ")
            .append(std::to_string(from_id_))
            .append(" , class: ")
            .append(util::As<RelationClass>(class_)->FromClass()->Name())
            .append(" ), ")
            .append("to: ")
            .append("( id: ")
            .append(std::to_string(to_id_))
            .append(" , class: ")
            .append(util::As<RelationClass>(class_)->ToClass()->Name())
            .append(" )")
            .append(
                attributes_object_.has_value()
                    ? std::string(", attributes: ").append(attributes_object_.value()->ToString())
                    : "")
            .append(" ) ");
    }

    [[nodiscard]] auto FromId() const noexcept -> Id {
        return from_id_;
    }
    [[nodiscard]] auto ToId() const noexcept -> Id {
        return to_id_;
    }
    [[nodiscard]] auto Attributes() const noexcept -> std::optional<Object::Ptr> {
        return attributes_object_;
    }
};
}  // namespace ts