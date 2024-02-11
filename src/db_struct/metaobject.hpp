#pragma once

#include "object.hpp"

namespace db {
class MetaObject : public ts::Object {
private:
    uint64_t magic_;
    size_t id_;
    std::shared_ptr<ts::Object> data_;

public:
    template <ts::ObjectLike O>
    MetaObject(uint64_t magic, size_t id, std::shared_ptr<O> data)
        : magic_(magic), id_(id), data_(data) {
    }

    size_t Size() const override {
        return sizeof(magic_) + sizeof(id_) + data_->Size();
    }
    mem::Offset Write(std::shared_ptr<mem::File>& file, mem::Offset offset) const override {
        file->Write<uint64_t>(magic_, offset);
        offset += sizeof(uint64_t);
        file->Write<size_t>(id_, offset);
        offset += sizeof(size_t);
        return data_->Write(file, offset);
    }
    void Read(std::shared_ptr<mem::File>& file, mem::Offset offset) override {
        magic_ = file->Read<uint64_t>(offset);
        offset += sizeof(uint64_t);
        id_ = file->Read<size_t>(offset);
        offset += sizeof(size_t);
        data_->Read(file, offset);
    }
    [[nodiscard]] std::string ToString() const override {
        return std::string("meta ")
            .append(std::to_string(id_))
            .append(": ")
            .append(data_->ToString());
    }

    MetaObject(std::shared_ptr<ts::Class> data_class, std::shared_ptr<mem::File>& file,
               mem::Offset offset) {

        magic_ = file->Read<uint64_t>(offset);
        offset += sizeof(uint64_t);
        id_ = file->Read<size_t>(offset);
        offset += sizeof(size_t);

        if (util::Is<ts::StructClass>(data_class)) {
            data_ = ts::ReadNew<ts::Struct>(util::As<ts::StructClass>(data_class), file, offset);
        } else if (util::Is<ts::StringClass>(data_class)) {
            data_ = ts::ReadNew<ts::String>(util::As<ts::StringClass>(data_class), file, offset);
        } else {

#define CREATE_PRIMITIVE(P)                                                                      \
    else if (util::Is<ts::PrimitiveClass<P>>(data_class)) {                                      \
        data_ = ts::ReadNew<ts::Primitive<P>>(util::As<ts::PrimitiveClass<P>>(data_class), file, \
                                              offset);                                           \
    }

            if (false) {
            }
            CREATE_PRIMITIVE(int)
            CREATE_PRIMITIVE(double)
            CREATE_PRIMITIVE(float)
            CREATE_PRIMITIVE(bool)
            CREATE_PRIMITIVE(unsigned int)
            CREATE_PRIMITIVE(short int)
            CREATE_PRIMITIVE(short unsigned int)
            CREATE_PRIMITIVE(long long int)
            CREATE_PRIMITIVE(long long unsigned int)
            CREATE_PRIMITIVE(long unsigned int)
            CREATE_PRIMITIVE(long int)
            CREATE_PRIMITIVE(char)
            CREATE_PRIMITIVE(signed char)
            CREATE_PRIMITIVE(unsigned char)
            CREATE_PRIMITIVE(wchar_t)
#undef CREATE_PRIMITIVE
            throw error::TypeError("Class can't be turned in metaobject");
        }
    }

    [[nodiscard]] size_t Id() const {
        return id_;
    }
};
}  // namespace db