#pragma once
#include <unordered_map>

#include "../mem/mem.h"

namespace db {

class Type {
public:
    virtual ~Type() = default;
    virtual size_t GetSize() {
        throw error::NotImplemented("Void Type");
    }
    virtual std::string GetName() {
        throw error::NotImplemented("Void Type");
    }
};

template <typename T>
class Primitive : public Type {
    std::string name_;
    T value_;

public:
    virtual ~Primitive() = default;
    explicit Primitive(const std::string& name, T value) : name_(name), value_(value) {
    }
    size_t GetSize() override {
        return sizeof(T);
    }
    std::string GetName() override {
        return name_;
    }
};

template <>
class Primitive<std::string> : public Type {
    std::string name_;
    std::string str_;

public:
    virtual ~Primitive() = default;
    explicit Primitive(const std::string& name, std::string&& str)
        : name_(name), str_(std::forward<std::string>(str)) {
    }
    explicit Primitive(const std::string& name, const std::string& str) : name_(name), str_(str) {
    }
    size_t GetSize() override {
        return str_.size();
    }
    std::string GetName() override {
        return name_;
    }
};

class Struct : public Type {

    using Fields = std::unordered_map<std::string, Type>;

    std::string name_;
    Fields fields_;

public:
    virtual ~Struct() = default;
    Struct(const std::string& name, const Fields& fields) : name_(name), fields_(fields) {
    }
    size_t GetSize() override {
        size_t size = 0;
        for (auto& field : fields_) {
            size += field.second.GetSize();
        }
        return size;
    }
    std::string GetName() override {
        return name_;
    }
};

}  // namespace db