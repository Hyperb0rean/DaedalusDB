#pragma once
#include <initializer_list>

#include "../mem/mem.h"

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
    virtual size_t GetSize() {
        throw error::NotImplemented("Void Type");
    }
    virtual std::string GetName() {
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
    size_t GetSize() override {
        return sizeof(T);
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
    size_t GetSize() override {
        return str_.size();
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
            std::dynamic_pointer_cast<StructType>(type_)->fields_.push_back(field->GetType());
        }
    }
    size_t GetSize() override {
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
};  // namespace types

}  // namespace types