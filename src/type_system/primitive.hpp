#pragma once

#include "object.hpp"
#include "primitive_class.hpp"

namespace ts {
template <typename T>
requires std::is_arithmetic_v<T>
class Primitive : public Object {
    T value_;

public:
    using Ptr = util::Ptr<Primitive<T>>;

    ~Primitive() = default;
    Primitive(const PrimitiveClass<T>::Ptr& argclass, T value) : value_(value) {
        this->class_ = argclass;
    }
    explicit Primitive(const PrimitiveClass<T>::Ptr& argclass) : value_(T{}) {
        this->class_ = argclass;
    }
    [[nodiscard]] size_t Size() const override {
        return sizeof(T);
    }
    [[nodiscard]] T Value() const {
        return value_;
    }
    [[nodiscard]] T& Value() {
        return value_;
    }
    mem::Offset Write(mem::File::Ptr& file, mem::Offset offset) const override {
        return file->Write<T>(value_, offset);
    }
    void Read(mem::File::Ptr& file, mem::Offset offset) override {
        value_ = file->Read<T>(offset);
    }
    [[nodiscard]] std::string ToString() const override {
        if constexpr (std::is_same_v<bool, T>) {
            return class_->Name() + ": " + (value_ ? "true" : "false");
        }
        return class_->Name() + ": " + std::to_string(value_);
    }
};
}  // namespace ts