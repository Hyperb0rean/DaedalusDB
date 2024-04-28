#pragma once

#include "error.hpp"
#include "object.hpp"
#include "struct_class.hpp"
#include "utils.hpp"

namespace ts {
class Struct : public Object {
    std::vector<Object::Ptr> fields_;

public:
    using Ptr = util::Ptr<Struct>;

    ~Struct() = default;
    Struct(const StructClass::Ptr& argclass) {
        this->class_ = argclass;
    }
    [[nodiscard]] auto Size() const -> size_t override {
        size_t size = 0;
        for (auto& field : fields_) {
            size += field->Size();
        }
        return size;
    }

    // Maybe should not be public
    template <ObjectLike O>
    auto AddFieldValue(const util::Ptr<O>& value) -> void {
        fields_.push_back(value);
    }

    // Maybe should not be public
    auto RemoveLAstFieldValue() -> void {
        fields_.pop_back();
    }

    [[nodiscard]] auto GetFields() const noexcept -> const std::vector<Object::Ptr>& {
        return fields_;
    }

    template <ObjectLike O>
    [[nodiscard]] auto GetField(std::string name) const -> util::Ptr<O> {
        for (auto& field : fields_) {
            if (field->GetClass()->Name() == name) {
                return util::As<O>(field);
            }
        }
        throw error::RuntimeError("No such field");
    }

    auto Write(mem::File::Ptr& file, mem::Offset offset) const -> mem::Offset override {
        mem::Offset new_offset = offset;
        for (auto& field : fields_) {
            field->Write(file, new_offset);
            new_offset += field->Size();
        }
        return new_offset;
    }
    auto Read(mem::File::Ptr& file, mem::Offset offset) -> void override {
        mem::Offset new_offset = offset;
        for (auto& field : fields_) {
            field->Read(file, new_offset);
            new_offset += field->Size();
        }
    }
    [[nodiscard]] auto ToString() const -> std::string override {
        std::string result = class_->Name() + ": { ";
        for (auto& field : fields_) {
            if (field != *fields_.begin()) {
                result.append(", ");
            }
            result.append(field->ToString());
        }
        result.append(" }");
        return result;
    }
};
}  // namespace ts