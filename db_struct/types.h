#pragma once
#include <initializer_list>
#include <iostream>

#include "../mem/mem.h"
#include "../util/utils.h"

namespace types {

struct Type {
    std::string name_;
    virtual ~Type() = default;
};
template <typename T>
struct PrimitiveType : public Type {
    explicit PrimitiveType(std::string name) {
        this->name_ = name;
    }
};
struct StructType : public Type {
    std::vector<std::shared_ptr<Type>> fields_;

    StructType(std::string name) {
        this->name_ = name;
    }
    template <typename ActualType>
    void AddField(ActualType field) requires std::derived_from<ActualType, Type> {
        fields_.push_back(std::make_shared<ActualType>(field));
    }
};

class Object {
protected:
    std::shared_ptr<Type> type_;

public:
    virtual std::shared_ptr<Type> GetType() {
        return type_;
    }
    virtual ~Object() = default;
    virtual size_t GetSize() const {
        throw error::NotImplemented("Void Type");
    }
    [[nodiscard]] virtual std::string GetName() const {
        throw error::NotImplemented("Void Type");
    }
    virtual mem::Offset Write(std::unique_ptr<mem::File>& file, mem::Offset offset) const {
        throw error::NotImplemented("Void Type");
    }
    virtual void Read(std::unique_ptr<mem::File>& file, mem::Offset offset) {
        throw error::NotImplemented("Void Type");
    }
    [[nodiscard]] virtual std::string ToString() const {
        throw error::NotImplemented("Void Type");
    }
};

template <typename T>
class Primitive : public Object {
    T value_;

public:
    virtual ~Primitive() = default;
    explicit Primitive(const std::string& name, T value) : value_(value) {
        this->type_ = std::make_shared<PrimitiveType<T>>(name);
    }
    [[nodiscard]] size_t GetSize() const override {
        return sizeof(T);
    }
    [[nodiscard]] T GetValue() const {
        return value_;
    }
    [[nodiscard]] T& GetValue() {
        return value_;
    }
    mem::Offset Write(std::unique_ptr<mem::File>& file, mem::Offset offset) const override {
        return file->Write<T>(value_, offset);
    }
    void Read(std::unique_ptr<mem::File>& file, mem::Offset offset) override {
        value_ = file->Read<T>(offset);
    }
    [[nodiscard]] std::string ToString() const override {
        return type_->name_ + ": " + std::to_string(value_);
    }
};

template <>
class Primitive<std::string> : public Object {
    std::string str_;

public:
    virtual ~Primitive() = default;
    explicit Primitive(std::string name, std::string&& str) : str_(std::forward<std::string>(str)) {
        this->type_ = std::make_shared<PrimitiveType<std::string>>(name);
    }
    explicit Primitive(std::string name, const std::string& str) : str_(str) {
        this->type_ = std::make_shared<PrimitiveType<std::string>>(name);
    }
    [[nodiscard]] size_t GetSize() const override {
        return str_.size() + 4;
    }
    [[nodiscard]] std::string GetValue() const {
        return str_;
    }
    [[nodiscard]] std::string& GetValue() {
        return str_;
    }
    mem::Offset Write(std::unique_ptr<mem::File>& file, mem::Offset offset) const override {
        auto new_offset = file->Write<uint32_t>(str_.size(), offset) + 4;
        return file->Write(str_, new_offset);
    }
    void Read(std::unique_ptr<mem::File>& file, mem::Offset offset) override {
        uint32_t size = file->Read<uint32_t>(offset);
        str_ = file->ReadString(offset + 4, size);
    }
    [[nodiscard]] std::string ToString() const override {
        return type_->name_ + ": \"" + str_ + "\"";
    }
};

class Struct : public Object {

    using Fields = std::vector<std::shared_ptr<Object>>;

    Fields fields_;

public:
    virtual ~Struct() = default;
    Struct(const std::string& name) {
        this->type_ = std::make_shared<StructType>(name);
        for (auto& field : fields_) {
            utils::As<StructType>(type_)->fields_.push_back(field->GetType());
        }
    }
    [[nodiscard]] size_t GetSize() const override {
        size_t size = 0;
        for (auto& field : fields_) {
            size += field->GetSize();
        }
        return size;
    }
    template <typename ActualType>
    void AddFieldValue(ActualType value) requires std::derived_from<ActualType, Object> {
        fields_.push_back(std::make_shared<ActualType>(value));
    }
    mem::Offset Write(std::unique_ptr<mem::File>& file, mem::Offset offset) const override {
        mem::Offset new_offset = offset;
        for (auto& field : fields_) {
            field->Write(file, new_offset);
            new_offset += field->GetSize();
        }
        return new_offset;
    }
    void Read(std::unique_ptr<mem::File>& file, mem::Offset offset) override {
        mem::Offset new_offset = offset;
        for (auto& field : fields_) {
            field->Read(file, new_offset);
            new_offset += field->GetSize();
        }
    }
    [[nodiscard]] std::string ToString() const override {
        std::string result = type_->name_ + ": { ";
        for (auto& field : fields_) {
            if (field != *fields_.begin()) {
                result += ", ";
            }
            result += field->ToString();
        }
        result += " }";
        return result;
    }
};

}  // namespace types