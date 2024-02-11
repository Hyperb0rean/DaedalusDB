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
    virtual size_t Size() const {
        throw error::NotImplemented("Void Class");
    }
    [[nodiscard]] virtual std::string Name() const {
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
    using SizeType = u_int32_t;

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
        DESERIALIZE_PRIMITIVE(signed char)
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
    size_t Size() const override {
        return serialized_.size() + sizeof(SizeType);
    }
    [[nodiscard]] std::string Name() const override {
        throw error::NotImplemented("Class Class");
    }
    mem::Offset Write(std::shared_ptr<mem::File>& file, mem::Offset offset) const override {
        auto new_offset = file->Write<SizeType>(serialized_.size(), offset) + sizeof(SizeType);
        return file->Write(serialized_, new_offset);
    }
    void Read(std::shared_ptr<mem::File>& file, mem::Offset offset) override {
        SizeType size = file->Read<SizeType>(offset);
        serialized_ = file->ReadString(offset + sizeof(SizeType), size);
        std::stringstream stream{serialized_};
        class_holder_ = Deserialize(stream);
    }
    [[nodiscard]] std::string ToString() const override {
        return serialized_;
    }

    template <ClassLike C>
    [[nodiscard]] bool Contains(std::shared_ptr<C> other_class) {
        return serialized_.contains(ClassObject(other_class).serialized_);
    }
};

template <typename T>
requires std::is_fundamental_v<T>
class Primitive : public Object {
    T value_;

public:
    ~Primitive() = default;
    Primitive(const std::shared_ptr<PrimitiveClass<T>>& argclass, T value) : value_(value) {
        this->class_ = argclass;
    }
    explicit Primitive(const std::shared_ptr<PrimitiveClass<T>>& argclass) : value_(T{}) {
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
    mem::Offset Write(std::shared_ptr<mem::File>& file, mem::Offset offset) const override {
        return file->Write<T>(value_, offset);
    }
    void Read(std::shared_ptr<mem::File>& file, mem::Offset offset) override {
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
    ~String() = default;

    String(const std::shared_ptr<StringClass>& argclass, std::string str) : str_(std::move(str)) {
        this->class_ = argclass;
    }
    explicit String(const std::shared_ptr<StringClass>& argclass) : str_("") {
        this->class_ = argclass;
    }
    [[nodiscard]] size_t Size() const override {
        return str_.size() + sizeof(SizeType);
    }
    [[nodiscard]] std::string Value() const {
        return str_;
    }
    [[nodiscard]] std::string& Value() {
        return str_;
    }
    mem::Offset Write(std::shared_ptr<mem::File>& file, mem::Offset offset) const override {
        auto new_offset = file->Write<SizeType>(str_.size(), offset) + sizeof(SizeType);
        return file->Write(str_, new_offset);
    }
    void Read(std::shared_ptr<mem::File>& file, mem::Offset offset) override {
        SizeType size = file->Read<SizeType>(offset);
        str_ = file->ReadString(offset + sizeof(SizeType), size);
    }
    [[nodiscard]] std::string ToString() const override {
        return class_->Name() + ": \"" + str_ + "\"";
    }
};

class Struct : public Object {
    std::vector<std::shared_ptr<Object>> fields_;

public:
    ~Struct() = default;
    Struct(const std::shared_ptr<StructClass>& argclass) {
        this->class_ = argclass;
    }
    [[nodiscard]] size_t Size() const override {
        size_t size = 0;
        for (auto& field : fields_) {
            size += field->Size();
        }
        return size;
    }

    template <ObjectLike O>
    void AddFieldValue(const std::shared_ptr<O>& value) {
        fields_.push_back(value);
    }

    [[nodiscard]] std::vector<std::shared_ptr<Object>> GetFields() const {
        return fields_;
    }

    mem::Offset Write(std::shared_ptr<mem::File>& file, mem::Offset offset) const override {
        mem::Offset new_offset = offset;
        for (auto& field : fields_) {
            field->Write(file, new_offset);
            new_offset += field->Size();
        }
        return new_offset;
    }
    void Read(std::shared_ptr<mem::File>& file, mem::Offset offset) override {
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
[[nodiscard]] std::shared_ptr<O> UnsafeNew(std::shared_ptr<C> object_class, Args&&... args) {

    if constexpr (std::is_same_v<O, Struct>) {
        auto new_object = std::make_shared<Struct>(object_class);
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
                        new_object->AddFieldValue(
                            UnsafeNew<Primitive<std::remove_reference_t<decltype(args)>>>(
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
    } else {
        return std::make_shared<O>(object_class, std::get<0>(std::tuple(args...)));
    }
    throw error::TypeError("Can't create object");
}

template <ObjectLike O, ClassLike C, typename... Args>
[[nodiscard]] std::shared_ptr<O> New(std::shared_ptr<C> object_class, Args&&... args) {

    if (object_class->Count() != sizeof...(Args)) {
        throw error::BadArgument("Wrong number of arguments");
    }

    return UnsafeNew<O>(object_class, args...);
}

template <ObjectLike O, ClassLike C>
[[nodiscard]] std::shared_ptr<O> DefaultNew(std::shared_ptr<C> object_class) {

    if constexpr (std::is_same_v<O, Struct>) {
        auto new_object = std::make_shared<Struct>(object_class);
        auto fields = util::As<StructClass>(object_class)->GetFields();
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            if (util::Is<StructClass>(*it)) {
                new_object->AddFieldValue(DefaultNew<Struct>(util::As<StructClass>(*it)));
            } else if (util::Is<StringClass>(*it)) {
                new_object->AddFieldValue(DefaultNew<String>(util::As<StringClass>(*it)));
            } else {
#define ADD_PRIMITIVE(P)                                                                       \
    if (util::Is<PrimitiveClass<P>>(*it)) {                                                    \
        new_object->AddFieldValue(DefaultNew<Primitive<P>>(util::As<PrimitiveClass<P>>(*it))); \
        continue;                                                                              \
    }
                ADD_PRIMITIVE(int)
                ADD_PRIMITIVE(double)
                ADD_PRIMITIVE(float)
                ADD_PRIMITIVE(bool)
                ADD_PRIMITIVE(unsigned int)
                ADD_PRIMITIVE(short int)
                ADD_PRIMITIVE(short unsigned int)
                ADD_PRIMITIVE(long long int)
                ADD_PRIMITIVE(long long unsigned int)
                ADD_PRIMITIVE(long unsigned int)
                ADD_PRIMITIVE(long int)
                ADD_PRIMITIVE(char)
                ADD_PRIMITIVE(signed char)
                ADD_PRIMITIVE(unsigned char)
                ADD_PRIMITIVE(wchar_t)
#undef ADD_PRIMITIVE

                throw error::TypeError("Type can't be defaulted");
            }
        }
        return new_object;
    } else if constexpr (std::is_same_v<O, String>) {
        return std::make_shared<String>(object_class);
    } else {
        return std::make_shared<O>(object_class);
    }
    throw error::TypeError("Can't create object");
}

template <ObjectLike O, ClassLike C>
[[nodiscard]] std::shared_ptr<O> ReadNew(std::shared_ptr<C> object_class,
                                         std::shared_ptr<mem::File>& file, mem::Offset offset) {

    if constexpr (std::is_same_v<O, Struct>) {
        auto new_object = DefaultNew<Struct>(object_class);
        new_object->Read(file, offset);
        return new_object;
    } else if constexpr (std::is_same_v<O, String>) {
        return std::make_shared<String>(object_class)->Read(file, offset);
    } else {
        return std::make_shared<O>(object_class)->Read(file, offset);
    }
    throw error::TypeError("Can't create object");
}

}  // namespace ts