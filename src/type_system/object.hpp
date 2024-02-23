#pragma once

#include <cstddef>
#include <cstdint>
#include <sstream>

#include "../mem/file.hpp"
#include "class.hpp"

#define DDB_PRIMITIVE_GENERATOR(MACRO) \
    MACRO(int)                         \
    MACRO(double)                      \
    MACRO(float)                       \
    MACRO(bool)                        \
    MACRO(unsigned int)                \
    MACRO(short int)                   \
    MACRO(short unsigned int)          \
    MACRO(long long int)               \
    MACRO(long long unsigned int)      \
    MACRO(long unsigned int)           \
    MACRO(long int)                    \
    MACRO(char)                        \
    MACRO(signed char)                 \
    MACRO(unsigned char)               \
    MACRO(unsigned long)               \
    MACRO(wchar_t)

namespace ts {

class Object {
protected:
    Class::Ptr class_;

public:
    using Ptr = util::Ptr<Object>;

    [[nodiscard]] virtual Class::Ptr GetClass() {
        return class_;
    }
    virtual ~Object() = default;
    [[nodiscard]] virtual size_t Size() const = 0;
    virtual mem::Offset Write(mem::File::Ptr& file, mem::Offset offset) const = 0;
    virtual void Read(mem::File::Ptr& file, mem::Offset offset) = 0;
    [[nodiscard]] virtual std::string ToString() const = 0;
};

template <typename O>
concept ObjectLike = std::derived_from<O, Object>;

class ClassObject : public Object {
    std::string serialized_;
    using SizeType = uint32_t;

    [[nodiscard]] std::string ReadString(std::stringstream& stream, char end) const {
        std::string result;
        char c;
        stream >> c;
        while (c != end) {
            result += c;
            stream >> c;
        }
        return result;
    }

    [[nodiscard]] Class::Ptr Deserialize(std::stringstream& stream) const {
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

            auto result = util::MakePtr<StructClass>(name);

            auto field = Deserialize(stream);
            while (field != nullptr) {
                result->AddField(field);
                field = Deserialize(stream);
            }
            return result;
        } else if (type == "string") {
            return util::MakePtr<StringClass>(ReadString(stream, '_'));
        } else {

            auto remove_spaces = [](const char* str) -> std::string {
                std::string s = str;
                return {s.begin(), remove_if(s.begin(), s.end(), isspace)};
            };

#define DDB_DESERIALIZE_PRIMITIVE(P)                                      \
    if (type == remove_spaces(#P)) {                                      \
        return util::MakePtr<PrimitiveClass<P>>(ReadString(stream, '_')); \
    }
            DDB_PRIMITIVE_GENERATOR(DDB_DESERIALIZE_PRIMITIVE)
#undef DESERIALIZE_PRIMITIVE

            throw error::NotImplemented("Unsupported for deserialization type " + type);
        }
    }

public:
    using Ptr = util::Ptr<ClassObject>;

    ~ClassObject() = default;
    ClassObject(){};

    ClassObject(const Class::Ptr& holder) {
        class_ = holder;
        serialized_ = class_->Serialize();
    }
    ClassObject(std::string string) : serialized_(std::move(string)) {
        std::stringstream stream(serialized_);
        class_ = Deserialize(stream);
    }
    [[nodiscard]] size_t Size() const override {
        return serialized_.size() + sizeof(SizeType);
    }
    mem::Offset Write(mem::File::Ptr& file, mem::Offset offset) const override {
        auto new_offset = file->Write<SizeType>(static_cast<SizeType>(serialized_.size()), offset) +
                          static_cast<mem::Offset>(sizeof(SizeType));
        return file->Write(serialized_, new_offset);
    }
    void Read(mem::File::Ptr& file, mem::Offset offset) override {
        SizeType size = file->Read<SizeType>(offset);
        serialized_ = file->ReadString(offset + static_cast<mem::Offset>(sizeof(SizeType)), size);
        std::stringstream stream{serialized_};
        class_ = Deserialize(stream);
    }
    [[nodiscard]] std::string ToString() const override {
        return serialized_;
    }

    template <ClassLike C>
    [[nodiscard]] bool Contains(util::Ptr<C> other_class) const {
        return serialized_.contains(ClassObject(other_class).serialized_);
    }
};

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

class String : public Object {
    std::string str_;
    using SizeType = u_int32_t;

public:
    using Ptr = util::Ptr<String>;

    ~String() = default;

    String(const StringClass::Ptr& argclass, std::string str) : str_(std::move(str)) {
        this->class_ = argclass;
    }
    explicit String(const StringClass::Ptr& argclass) : str_("") {
        this->class_ = argclass;
    }
    [[nodiscard]] size_t Size() const override {
        return str_.size() + sizeof(SizeType);
    }
    [[nodiscard]] std::string_view Value() const {
        return str_;
    }
    [[nodiscard]] std::string& Value() {
        return str_;
    }
    mem::Offset Write(mem::File::Ptr& file, mem::Offset offset) const override {
        auto new_offset = file->Write<SizeType>(static_cast<SizeType>(str_.size()), offset) +
                          static_cast<mem::Offset>(sizeof(SizeType));
        return file->Write(str_, new_offset);
    }
    void Read(mem::File::Ptr& file, mem::Offset offset) override {
        SizeType size = file->Read<SizeType>(offset);
        str_ = file->ReadString(offset + static_cast<mem::Offset>(sizeof(SizeType)), size);
    }
    [[nodiscard]] std::string ToString() const override {
        return class_->Name() + ": \"" + str_ + "\"";
    }
};

class Struct : public Object {
    std::vector<Object::Ptr> fields_;

    template <ObjectLike O>
    void AddFieldValue(const util::Ptr<O>& value) {
        fields_.push_back(value);
    }

public:
    using Ptr = util::Ptr<Struct>;

    template <ObjectLike O, ClassLike C, typename... Args>
    friend util::Ptr<O> UnsafeNew(util::Ptr<C> object_class, Args&&... args);
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

template <ObjectLike O, ClassLike C, typename... Args>
[[nodiscard]] util::Ptr<O> UnsafeNew(util::Ptr<C> object_class, Args&&... args) {
    if constexpr (std::is_same_v<O, Struct>) {
        auto new_object = util::MakePtr<Struct>(object_class);
        auto it = util::As<StructClass>(object_class)->GetFields().begin();
        (
            [&it, &args, &new_object] {
                auto class_ = *it++;
                if (util::Is<StructClass>(class_)) {
                    new_object->AddFieldValue(
                        UnsafeNew<Struct>(util::As<StructClass>(class_), args));
                } else if (util::Is<StringClass>(class_)) {
                    if constexpr (std::is_convertible_v<decltype(args), std::string_view>) {
                        new_object->AddFieldValue(
                            UnsafeNew<String>(util::As<StringClass>(class_), args));
                    }
                } else {
                    if constexpr (!std::is_convertible_v<std::remove_reference_t<decltype(args)>,
                                                         std::string_view>) {
#define DDB_ADD_PRIMITIVE(P)                                                     \
    else if (util::Is<PrimitiveClass<P>>(class_)) {                              \
        new_object->AddFieldValue(                                               \
            UnsafeNew<Primitive<P>>(util::As<PrimitiveClass<P>>(class_), args)); \
    }

                        if (false) {
                        }
                        DDB_PRIMITIVE_GENERATOR(DDB_ADD_PRIMITIVE)
#undef DDB_ADD_PRIMITIVE
                    }
                }
            }(),
            ...);
        return new_object;
    } else if constexpr (std::is_same_v<O, String>) {
        return util::MakePtr<String>(object_class, std::move(std::get<0>(std::tuple(args...))));
    } else {
        return util::MakePtr<O>(object_class, std::get<0>(std::tuple(args...)));
    }
    throw error::TypeError("Can't create object");
}

template <ObjectLike O, ClassLike C, typename... Args>
[[nodiscard]] util::Ptr<O> New(util::Ptr<C> object_class, Args&&... args) {

    if (object_class->Count() != sizeof...(Args)) {
        throw error::BadArgument("Wrong number of arguments");
    }

    return UnsafeNew<O>(object_class, args...);
}

template <ObjectLike O, ClassLike C>
[[nodiscard]] util::Ptr<O> DefaultNew(util::Ptr<C> object_class) {

    if constexpr (std::is_same_v<O, Struct>) {
        auto new_object = util::MakePtr<Struct>(object_class);
        auto fields = util::As<StructClass>(object_class)->GetFields();
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            if (util::Is<StructClass>(*it)) {
                new_object->AddFieldValue(DefaultNew<Struct>(util::As<StructClass>(*it)));
            } else if (util::Is<StringClass>(*it)) {
                new_object->AddFieldValue(DefaultNew<String>(util::As<StringClass>(*it)));
            } else {
#define DDB_ADD_PRIMITIVE(P)                                                                   \
    if (util::Is<PrimitiveClass<P>>(*it)) {                                                    \
        new_object->AddFieldValue(DefaultNew<Primitive<P>>(util::As<PrimitiveClass<P>>(*it))); \
        continue;                                                                              \
    }
                DDB_PRIMITIVE_GENERATOR(DDB_ADD_PRIMITIVE)
#undef DDB_ADD_PRIMITIVE

                throw error::TypeError("Class can't be defaulted");
            }
        }
        return new_object;
    } else if constexpr (std::is_same_v<O, String>) {
        return util::MakePtr<String>(object_class);
    } else {
        return util::MakePtr<O>(object_class);
    }
    throw error::TypeError("Can't create object");
}

template <ObjectLike O, ClassLike C>
[[nodiscard]] util::Ptr<O> ReadNew(util::Ptr<C> object_class, mem::File::Ptr& file,
                                   mem::Offset offset) {

    auto new_object = DefaultNew<O, C>(object_class);
    new_object->Read(file, offset);
    return new_object;
}

}  // namespace ts