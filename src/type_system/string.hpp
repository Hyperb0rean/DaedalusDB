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
    [[nodiscard]] auto Size() const -> size_t override {
        return str_.size() + sizeof(SizeType);
    }
    [[nodiscard]] auto Value() const noexcept -> std::string_view {
        return str_;
    }
    [[nodiscard]] auto Value() noexcept -> std::string& {
        return str_;
    }
    auto Write(mem::File::Ptr& file, mem::Offset offset) const -> mem::Offset override {
        auto new_offset = file->Write<SizeType>(static_cast<SizeType>(str_.size()), offset) +
                          static_cast<mem::Offset>(sizeof(SizeType));
        return file->Write(str_, new_offset);
    }
    auto Read(mem::File::Ptr& file, mem::Offset offset) -> void override {
        SizeType size = file->Read<SizeType>(offset);
        str_ = file->ReadString(offset + static_cast<mem::Offset>(sizeof(SizeType)), size);
    }
    [[nodiscard]] auto ToString() const -> std::string override {
        return class_->Name() + ": \"" + str_ + "\"";
    }
};

}  // namespace ts