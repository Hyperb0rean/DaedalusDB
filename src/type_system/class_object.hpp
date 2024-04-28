#pragma once

#include <sstream>

#include "object.hpp"
#include "primitive_class.hpp"
#include "relation_class.hpp"
#include "string_class.hpp"
#include "struct_class.hpp"

namespace ts {
class ClassObject : public Object {
    std::string serialized_;
    using SizeType = uint32_t;

    [[nodiscard]] auto ReadString(std::stringstream& stream, char end) const -> std::string {
        std::string result;
        char c;
        stream >> c;
        while (c != end) {
            result += c;
            stream >> c;
        }
        return result;
    }

    [[nodiscard]] auto Deserialize(std::stringstream& stream) const -> Class::Ptr {
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
            stream >> del;
            if (del != '_') {
                throw error::TypeError("Can't read correct type by this address");
            }
            return result;
        } else if (type == "relation") {
            auto name = ReadString(stream, '_');
            auto from = Deserialize(stream);
            auto to = Deserialize(stream);
            stream >> del;
            if (del == '1') {
                return util::MakePtr<RelationClass>(name, from, to, Deserialize(stream));
            } else if (del == '_') {
                return util::MakePtr<RelationClass>(name, from, to);
            } else {
                throw error::TypeError("Can't read correct type by this address");
            }
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
    [[nodiscard]] auto Size() const -> size_t override {
        return serialized_.size() + sizeof(SizeType);
    }
    auto Write(mem::File::Ptr& file, mem::Offset offset) const -> mem::Offset override {
        auto new_offset = file->Write<SizeType>(static_cast<SizeType>(serialized_.size()), offset) +
                          static_cast<mem::Offset>(sizeof(SizeType));
        return file->Write(serialized_, new_offset);
    }
    auto Read(mem::File::Ptr& file, mem::Offset offset) -> void override {
        SizeType size = file->Read<SizeType>(offset);
        serialized_ = file->ReadString(offset + static_cast<mem::Offset>(sizeof(SizeType)), size);
        std::stringstream stream{serialized_};
        class_ = Deserialize(stream);
    }
    [[nodiscard]] auto ToString() const -> std::string override {
        return serialized_;
    }

    template <ClassLike C>
    [[nodiscard]] auto Contains(util::Ptr<C> other_class) const noexcept -> bool {
        return serialized_.contains(ClassObject(other_class).serialized_);
    }
};
}  // namespace ts