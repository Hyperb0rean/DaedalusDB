#pragma once

#include <variant>

#include "object.hpp"

namespace db {

enum class ObjectState { kFree, kValid, kInvalid };

[[nodiscard]] inline std::string ObjectStateToString(ObjectState state) {
    switch (state) {
        case ObjectState::kFree:
            return "free";
        case ObjectState::kValid:
            return "valid";
        case ObjectState::kInvalid:
            return "invalid";
        default:
            return "";
    }
}

using ObjectId = size_t;

class Node : public ts::Object {
private:
    mem::Magic magic_;
    std::variant<mem::PageOffset, ObjectId> meta_;
    std::shared_ptr<ts::Object> data_;
    ObjectState state_;

public:
    template <ts::ObjectLike O>
    Node(mem::Magic magic, ObjectId id, std::shared_ptr<O> data)
        : magic_(magic), meta_(id), data_(data), state_(ObjectState::kValid) {
    }

    size_t Size() const override {
        switch (state_) {
            case ObjectState::kFree: {
                if (data_->GetClass()->Size().has_value()) {
                    return sizeof(mem::Magic) + sizeof(ObjectId);
                } else {
                    return sizeof(mem::Magic) + sizeof(mem::PageOffset);
                }
            }
            case ObjectState::kValid:
                return sizeof(mem::Magic) + sizeof(ObjectId) + data_->Size();
            case ObjectState::kInvalid:
                throw error::BadArgument("Invalid object has no size");
            default:
                throw error::RuntimeError("Invalid state");
        }
    }

    std::optional<size_t> OptionalSize() const {
        if (state_ == ObjectState::kInvalid) {
            throw error::BadArgument("Invalid object has no theoretical size");
        }

        if (data_->GetClass()->Size().has_value()) {
            return sizeof(mem::Magic) + sizeof(ObjectId) + data_->GetClass()->Size().value();
        } else {
            return std::nullopt;
        }
    }

    mem::Offset Write(std::shared_ptr<mem::File>& file, mem::Offset offset) const override {
        switch (state_) {
            case ObjectState::kFree:
                file->Write<mem::Magic>(~magic_, offset);
                offset += sizeof(mem::Magic);
                if (data_->GetClass()->Size().has_value()) {
                    return file->Write(std::get<ObjectId>(meta_), offset);
                } else {
                    return file->Write(std::get<mem::PageOffset>(meta_), offset);
                }
            case ObjectState::kValid:
                file->Write<mem::Magic>(magic_, offset);
                offset += sizeof(mem::Magic);
                file->Write(std::get<ObjectId>(meta_), offset);
                offset += sizeof(ObjectId);
                return data_->Write(file, offset);
            case ObjectState::kInvalid:
                throw error::BadArgument("Trying to write invalid object");
            default:
                throw error::RuntimeError("Invalid state");
        }
    }
    void Read(std::shared_ptr<mem::File>& file, mem::Offset offset) override {
        auto magic = file->Read<mem::Magic>(offset);
        offset += sizeof(mem::Magic);

        if (magic == magic_) {
            state_ = ObjectState::kValid;
            meta_ = file->Read<ObjectId>(offset);
            offset += sizeof(ObjectId);
            data_->Read(file, offset);
        } else if (magic == ~magic_) {
            state_ = ObjectState::kFree;
            if (data_->GetClass()->Size().has_value()) {
                meta_ = file->Read<ObjectId>(offset);
            } else {
                meta_ = file->Read<mem::PageOffset>(offset);
            }
        } else {
            state_ = ObjectState::kInvalid;
        }
    }
    [[nodiscard]] std::string ToString() const override {
        return std::string("NODE: ")
            .append("state: ")
            .append(ObjectStateToString(state_))
            .append(std::holds_alternative<ObjectId>(meta_)
                        ? std::string(", id: ").append(std::to_string(std::get<ObjectId>(meta_)))
                        : std::string(", jumper: ")
                              .append(std::to_string(std::get<mem::PageOffset>(meta_))))
            .append(", data: { ")
            .append(state_ == ObjectState::kValid ? data_->ToString() : ObjectStateToString(state_))
            .append(" } ");
    }

    Node(mem::Magic magic, std::shared_ptr<ts::Class> data_class, std::shared_ptr<mem::File>& file,
         mem::Offset offset)
        : magic_(magic) {

        auto read_magic = file->Read<mem::Magic>(offset);
        offset += sizeof(mem::Magic);

        if (read_magic == magic_) {
            state_ = ObjectState::kValid;
            meta_ = file->Read<ObjectId>(offset);
            offset += sizeof(ObjectId);

            if (util::Is<ts::StructClass>(data_class)) {
                data_ =
                    ts::ReadNew<ts::Struct>(util::As<ts::StructClass>(data_class), file, offset);
            } else if (util::Is<ts::StringClass>(data_class)) {
                data_ =
                    ts::ReadNew<ts::String>(util::As<ts::StringClass>(data_class), file, offset);
            } else {

#define DDB_CREATE_PRIMITIVE(P)                                                                  \
    else if (util::Is<ts::PrimitiveClass<P>>(data_class)) {                                      \
        data_ = ts::ReadNew<ts::Primitive<P>>(util::As<ts::PrimitiveClass<P>>(data_class), file, \
                                              offset);                                           \
    }

                if (false) {
                }
                DDB_PRIMITIVE_GENERATOR(DDB_CREATE_PRIMITIVE)
#undef CREATE_PRIMITIVE
                throw error::TypeError("Class can't be turned in Node");
            }
        } else if (read_magic == ~magic_) {
            state_ = ObjectState::kFree;
            if (data_class->Size().has_value()) {
                meta_ = file->Read<ObjectId>(offset);
            } else {
                meta_ = file->Read<mem::PageOffset>(offset);
            }
        } else {
            state_ = ObjectState::kInvalid;
        }
    }

    [[nodiscard]] ObjectId Id() const {
        if (state_ == ObjectState::kInvalid || !data_->GetClass()->Size().has_value()) {
            throw error::BadArgument("Node has no id");
        }
        return std::get<ObjectId>(meta_);
    }

    [[nodiscard]] mem::PageOffset Jumper() const {
        if (state_ == ObjectState::kInvalid || data_->GetClass()->Size().has_value()) {
            throw error::BadArgument("Node has no jumper");
        }
        return std::get<mem::PageOffset>(meta_);
    }

    [[nodiscard]] ObjectState State() const {
        return state_;
    }
};
}  // namespace db