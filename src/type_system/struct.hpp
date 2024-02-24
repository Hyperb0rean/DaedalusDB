#pragma once

#include "object.hpp"
#include "struct_class.hpp"

namespace ts {
class Struct : public Object {
    std::vector<Object::Ptr> fields_;

    template <ObjectLike O>
    void AddFieldValue(const util::Ptr<O>& value) {
        fields_.push_back(value);
    }

public:
    using Ptr = util::Ptr<Struct>;

    template <ObjectLike O, ClassLike C>
    friend util::Ptr<O> UnsafeNew(util::Ptr<C> object_class,
                                  std::initializer_list<std::any>::iterator& args);
    template <ObjectLike O, ClassLike C>
    friend util::Ptr<O> DefaultNew(util::Ptr<C> object_class);

    ~Struct() = default;
    Struct(const StructClass::Ptr& argclass) {
        this->class_ = argclass;
    }
    [[nodiscard]] size_t Size() const override {
        size_t size = 0;
        for (auto& field : fields_) {
            size += field->Size();
        }
        return size;
    }

    [[nodiscard]] const std::vector<Object::Ptr> GetFields() const {
        return fields_;
    }

    template <ObjectLike O>
    [[nodiscard]] util::Ptr<O> GetField(std::string name) const {
        for (auto& field : fields_) {
            if (field->GetClass()->Name() == name) {
                return util::As<O>(field);
            }
        }
        throw error::RuntimeError("No such field");
    }

    mem::Offset Write(mem::File::Ptr& file, mem::Offset offset) const override {
        mem::Offset new_offset = offset;
        for (auto& field : fields_) {
            field->Write(file, new_offset);
            new_offset += field->Size();
        }
        return new_offset;
    }
    void Read(mem::File::Ptr& file, mem::Offset offset) override {
        mem::Offset new_offset = offset;
        for (auto& field : fields_) {
            field->Read(file, new_offset);
            new_offset += field->Size();
        }
    }
    [[nodiscard]] std::string ToString() const override {
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