#pragma once
#include <sstream>
#include <typeinfo>

#include "file.h"
#include "utils.h"

namespace types {

// TODO: May be compiler dependent
template <typename T>
constexpr std::string_view type_name() {
    constexpr auto prefix = std::string_view{"[with T = "};
    constexpr auto suffix = std::string_view{";"};
    constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
    constexpr auto start = function.find(prefix) + prefix.size();
    constexpr auto end = function.rfind(suffix);
    static_assert(start < end);
    constexpr auto result = function.substr(start, (end - start));
    return result;
}

struct Class {
    std::string name_;
    virtual ~Class() = default;
    [[nodiscard]] virtual std::string Serialize() const = 0;
};
template <typename T>
struct PrimitiveClass : public Class {
    explicit PrimitiveClass(std::string name) {
        this->name_ = name;
    }
    [[nodiscard]] std::string Serialize() const override {
        std::string result = "_";
        result += type_name<T>();
        result += "@";
        result += name_;
        result += "_";
        return result;
    }
};

struct StringClass : public Class {
    explicit StringClass(std::string name) {
        this->name_ = name;
    }
    [[nodiscard]] std::string Serialize() const override {
        return "_string@" + name_ + "_";
    }
};

struct StructClass : public Class {
    std::vector<std::shared_ptr<Class>> fields_;

    StructClass(std::string name) {
        this->name_ = name;
    }
    template <typename ActualClass>
    void AddField(ActualClass field) requires std::derived_from<ActualClass, Class> {
        fields_.push_back(std::make_shared<ActualClass>(field));
    }

    template <typename ActualClass>
    void AddField(
        const std::shared_ptr<ActualClass>& field) requires std::derived_from<ActualClass, Class> {
        fields_.push_back(field);
    }

    [[nodiscard]] std::string Serialize() const override {
        auto result = "_struct@" + name_ + "_<";
        for (auto& field : fields_) {
            result += field->Serialize();
        }
        result += ">";
        return result;
    }
};

class Object {
protected:
    std::shared_ptr<Class> class_;

public:
    virtual std::shared_ptr<Class> GetClass() {
        return class_;
    }
    virtual ~Object() = default;
    virtual size_t GetSize() const {
        throw error::NotImplemented("Void Type");
    }
    [[nodiscard]] virtual std::string GetName() const {
        throw error::NotImplemented("Void Type");
    }
    virtual mem::Offset Write(std::shared_ptr<mem::File>& file, mem::Offset offset) const {
        throw error::NotImplemented("Void Type");
    }
    virtual void Read(std::shared_ptr<mem::File>& file, mem::Offset offset) {
        throw error::NotImplemented("Void Type");
    }
    [[nodiscard]] virtual std::string ToString() const {
        throw error::NotImplemented("Void Type");
    }
};

class ClassObject : public Object {
    std::shared_ptr<Class> class_holder_;
    std::string serialized_;

    std::string ReadString(std::stringstream& stream, char end) {
        std::string result;
        char c;
        stream >> c;
        while (c != end) {
            result += c;
            stream >> c;
        }
        return result;
    }

    std::shared_ptr<Class> Deserialize(std::stringstream& stream) {
        char del;
        stream >> del;
        if (del == '>') {
            return nullptr;
        }
        if (del != '_') {
            throw error::TypeError("Can't read correct type by this address");
        }

        auto type = ReadString(stream, '@');
        if (type == "struct") {
            auto name = ReadString(stream, '_');
            stream >> del;
            if (del != '<') {
                throw error::TypeError("Can't read correct type by this address");
            }

            auto result = std::make_shared<StructClass>(name);

            auto field = Deserialize(stream);
            while (field != nullptr) {
                result->AddField(field);
                field = Deserialize(stream);
            }
            return result;
        } else if (type == "string") {
            return std::make_shared<StringClass>(ReadString(stream, '_'));
        } else if (type == "int") {
            return std::make_shared<PrimitiveClass<int>>(ReadString(stream, '_'));
        } else if (type == "double") {
            return std::make_shared<PrimitiveClass<double>>(ReadString(stream, '_'));
        } else if (type == "bool") {
            return std::make_shared<PrimitiveClass<bool>>(ReadString(stream, '_'));
        } else {
            throw error::TypeError("Can't read correct type by this address");
        }
    }

public:
    ~ClassObject() = default;
    ClassObject(){};
    virtual std::shared_ptr<Class> GetClass() {
        throw error::NotImplemented("Class Type");
    }
    ClassObject(const std::shared_ptr<Class>& holder) : class_holder_(holder) {
        serialized_ = class_holder_->Serialize();
    }
    virtual size_t GetSize() const {
        return serialized_.size() + 4;
    }
    [[nodiscard]] virtual std::string GetName() const {
        throw error::NotImplemented("Class Type");
    }
    virtual mem::Offset Write(std::shared_ptr<mem::File>& file, mem::Offset offset) const {
        auto new_offset = file->Write<uint32_t>(serialized_.size(), offset) + 4;
        return file->Write(serialized_, new_offset);
    }
    virtual void Read(std::shared_ptr<mem::File>& file, mem::Offset offset) {
        uint32_t size = file->Read<uint32_t>(offset);
        serialized_ = file->ReadString(offset + 4, size);
        std::stringstream stream{serialized_};
        class_holder_ = Deserialize(stream);
    }
    [[nodiscard]] virtual std::string ToString() const {
        return "class: " + serialized_;
    }
};

// TODO: make concept only to pass Standart layout types.
template <typename T>
class Primitive : public Object {
    T value_;

public:
    virtual ~Primitive() = default;
    explicit Primitive(const std::string& name, T value) : value_(value) {
        this->class_ = std::make_shared<PrimitiveClass<T>>(name);
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
    mem::Offset Write(std::shared_ptr<mem::File>& file, mem::Offset offset) const override {
        return file->Write<T>(value_, offset);
    }
    void Read(std::shared_ptr<mem::File>& file, mem::Offset offset) override {
        value_ = file->Read<T>(offset);
    }
    [[nodiscard]] std::string ToString() const override {
        return class_->name_ + ": " + std::to_string(value_);
    }
};

class String : public Object {
    std::string str_;

public:
    virtual ~String() = default;
    explicit String(std::string name, std::string&& str) : str_(std::forward<std::string>(str)) {
        this->class_ = std::make_shared<StringClass>(name);
    }
    explicit String(std::string name, const std::string& str) : str_(str) {
        this->class_ = std::make_shared<StringClass>(name);
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
    mem::Offset Write(std::shared_ptr<mem::File>& file, mem::Offset offset) const override {
        auto new_offset = file->Write<uint32_t>(str_.size(), offset) + 4;
        return file->Write(str_, new_offset);
    }
    void Read(std::shared_ptr<mem::File>& file, mem::Offset offset) override {
        uint32_t size = file->Read<uint32_t>(offset);
        str_ = file->ReadString(offset + 4, size);
    }
    [[nodiscard]] std::string ToString() const override {
        return class_->name_ + ": \"" + str_ + "\"";
    }
};

class Struct : public Object {

    using Fields = std::vector<std::shared_ptr<Object>>;

    Fields fields_;

public:
    virtual ~Struct() = default;
    Struct(const std::string& name) {
        this->class_ = std::make_shared<StructClass>(name);
        for (auto& field : fields_) {
            utils::As<StructClass>(class_)->fields_.push_back(field->GetClass());
        }
    }
    [[nodiscard]] size_t GetSize() const override {
        size_t size = 0;
        for (auto& field : fields_) {
            size += field->GetSize();
        }
        return size;
    }
    template <typename ActualObject>
    void AddFieldValue(ActualObject value) requires std::derived_from<ActualObject, Object> {
        fields_.push_back(std::make_shared<ActualObject>(value));
    }

    template <typename ActualObject>
    void AddFieldValue(const std::shared_ptr<ActualObject>& value) requires
        std::derived_from<ActualObject, Class> {
        fields_.push_back(value);
    }

    mem::Offset Write(std::shared_ptr<mem::File>& file, mem::Offset offset) const override {
        mem::Offset new_offset = offset;
        for (auto& field : fields_) {
            field->Write(file, new_offset);
            new_offset += field->GetSize();
        }
        return new_offset;
    }
    void Read(std::shared_ptr<mem::File>& file, mem::Offset offset) override {
        mem::Offset new_offset = offset;
        for (auto& field : fields_) {
            field->Read(file, new_offset);
            new_offset += field->GetSize();
        }
    }
    [[nodiscard]] std::string ToString() const override {
        std::string result = class_->name_ + ": { ";
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