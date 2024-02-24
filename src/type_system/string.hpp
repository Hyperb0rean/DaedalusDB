#pragma once

#include "object.hpp"
#include "string_class.hpp"

namespace ts {
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

}  // namespace ts