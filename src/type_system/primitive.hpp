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
    [[nodiscard]] auto Size() const -> size_t override {
        return sizeof(T);
    }
    [[nodiscard]] auto Value() const noexcept -> T {
        return value_;
    }
    [[nodiscard]] auto Value() noexcept -> T& {
        return value_;
    }
    auto Write(mem::File::Ptr& file, mem::Offset offset) const -> mem::Offset override {
        return file->Write<T>(value_, offset);
    }
    auto Read(mem::File::Ptr& file, mem::Offset offset) -> void override {
        value_ = file->Read<T>(offset);
    }
    [[nodiscard]] auto ToString() const -> std::string override {
        if constexpr (std::is_same_v<bool, T>) {
            return class_->Name() + ": " + (value_ ? "true" : "false");
        } else {
            return class_->Name() + ": " + std::to_string(value_);
        }
    }
};
}  // namespace ts