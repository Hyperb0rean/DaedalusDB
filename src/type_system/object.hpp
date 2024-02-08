#pragma once

#include "class.hpp"

namespace ts {

class Object {
protected:
    std::shared_ptr<Class> class_;

public:
    virtual std::shared_ptr<Class> GetClass() {
        return class_;
    }
    virtual ~Object() = default;
    virtual size_t GetSize() const {
        throw error::NotImplemented("Void Class");
    }
    [[nodiscard]] virtual std::string GetName() const {
        throw error::NotImplemented("Void Class");
    }
    virtual mem::Offset Write(std::shared_ptr<mem::File>& file, mem::Offset offset) const {
        throw error::NotImplemented("Void Class");
    }
    virtual void Read(std::shared_ptr<mem::File>& file, mem::Offset offset) {
        throw error::NotImplemented("Void Class");
    }
    [[nodiscard]] virtual std::string ToString() const {
        throw error::NotImplemented("Void Class");
    }
};

template <typename O>
concept ObjectLike = std::derived_from<O, Object>;

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

        auto remove_spaces = [](const char* str) -> std::string {
            std::string s = str;
            return {s.begin(), remove_if(s.begin(), s.end(), isspace)};
        };

#define DESERIALIZE_PRIMITIVE(P)                                             \
    else if (type == remove_spaces(#P)) {                                    \
        return std::make_shared<PrimitiveClass<P>>(ReadString(stream, '_')); \
    }

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
        }
        DESERIALIZE_PRIMITIVE(int)
        DESERIALIZE_PRIMITIVE(double)
        DESERIALIZE_PRIMITIVE(float)
        DESERIALIZE_PRIMITIVE(bool)
        DESERIALIZE_PRIMITIVE(unsigned int)
        DESERIALIZE_PRIMITIVE(short int)
        DESERIALIZE_PRIMITIVE(short unsigned int)
        DESERIALIZE_PRIMITIVE(long long int)
        DESERIALIZE_PRIMITIVE(long long unsigned int)
        DESERIALIZE_PRIMITIVE(long unsigned int)
        DESERIALIZE_PRIMITIVE(long int)
        DESERIALIZE_PRIMITIVE(char)
        DESERIALIZE_PRIMITIVE(unsigned char)
        DESERIALIZE_PRIMITIVE(wchar_t)
        else {
            throw error::NotImplemented("Unsupported for deserialization type");
        }

#undef DESERIALIZE_PRIMITIVE
    }

public:
    ~ClassObject() = default;
    ClassObject(){};
    virtual std::shared_ptr<Class> GetClass() {
        return class_holder_;
    }
    ClassObject(const std::shared_ptr<Class>& holder) : class_holder_(holder) {
        serialized_ = class_holder_->Serialize();
    }
    ClassObject(std::string string) : serialized_(string) {
        std::stringstream stream(serialized_);
        class_holder_ = Deserialize(stream);
    }
    virtual size_t GetSize() const {
        return serialized_.size() + 4;
    }
    [[nodiscard]] virtual std::string GetName() const {
        throw error::NotImplemented("Class Class");
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
        return serialized_;
    }
};  // namespace ts

template <typename T>
requires std::is_fundamental_v<T>
class Primitive : public Object {
    T value_;

public:
    virtual ~Primitive() = default;
    explicit Primitive(const std::shared_ptr<PrimitiveClass<T>>& argclass, T value)
        : value_(value) {
        this->class_ = argclass;
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
    explicit String(const std::shared_ptr<StringClass>& argclass, std::string str)
        : str_(std::move(str)) {
        this->class_ = argclass;
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

    std::vector<std::shared_ptr<Object>> fields_;

public:
    virtual ~Struct() = default;
    Struct(const std::shared_ptr<StructClass>& argclass) {
        this->class_ = argclass;
    }
    [[nodiscard]] size_t GetSize() const override {
        size_t size = 0;
        for (auto& field : fields_) {
            size += field->GetSize();
        }
        return size;
    }

    template <ObjectLike ActualObject>
    void AddFieldValue(const std::shared_ptr<ActualObject>& value) {
        fields_.push_back(value);
    }

    [[nodiscard]] std::vector<std::shared_ptr<Object>> GetFields() const {
        return fields_;
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
                result.append(", ");
            }
            result.append(field->ToString());
        }
        result.append(" }");
        return result;
    }
};

template <ObjectLike O, ClassLike C, typename... Args>
[[nodiscard]] std::shared_ptr<O> New(const std::shared_ptr<C>& object_class, Args&&... args) {

    if constexpr (std::is_same_v<O, Struct>) {
        auto new_object = std::make_shared<Struct>(object_class);
        auto it = util::As<StructClass>(object_class)->fields_.begin();
        (
            [&it, &args, &new_object] {
                auto class_ = *it++;
                if (util::Is<StructClass>(class_)) {
                    new_object->AddFieldValue(New<Struct>(util::As<StructClass>(class_), args));
                } else if (util::Is<StringClass>(class_)) {
                    if constexpr (std::is_convertible_v<decltype(args), std::string_view>) {
                        new_object->AddFieldValue(New<String>(util::As<StringClass>(class_), args));
                    }
                } else {
                    if constexpr (!std::is_convertible_v<std::remove_reference_t<decltype(args)>,
                                                         std::string_view>) {
                        new_object->AddFieldValue(
                            New<Primitive<std::remove_reference_t<decltype(args)>>>(
                                util::As<PrimitiveClass<std::remove_reference_t<decltype(args)>>>(
                                    class_),
                                args));
                    }
                }
            }(),
            ...);
        return new_object;
    } else if constexpr (std::is_same_v<O, String>) {
        return std::make_shared<String>(object_class, std::move(std::get<0>(std::tuple(args...))));
    } else if constexpr (!std::is_convertible_v<std::tuple_element_t<0, std::tuple<Args...>>,
                                                std::string_view>) {
        return std::make_shared<
            Primitive<std::remove_reference_t<std::tuple_element_t<0, std::tuple<Args...>>>>>(
            object_class, std::get<0>(std::tuple(args...)));
    }
    throw error::TypeError("Can't create object");
}

}  // namespace ts