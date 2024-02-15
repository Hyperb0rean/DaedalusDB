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
    // Node(mem::Magic magic, mem::PageOffset free)
    //     : magic_(magic), meta_(free), state_(ObjectState::kFree) {
    // }

    template <ts::ObjectLike O>
    Node(mem::Magic magic, ObjectId id, std::shared_ptr<O> data)
        : magic_(magic), meta_(id), data_(data), state_(ObjectState::kValid) {
    }

    size_t Size() const override {
        switch (state_) {
            case ObjectState::kFree: {
                return sizeof(mem::Magic) + sizeof(mem::PageOffset);
            }
            case ObjectState::kValid:
                return sizeof(mem::Magic) + sizeof(ObjectId) + data_->Size();
            case ObjectState::kInvalid:
                throw error::BadArgument("Invalid object has no size");
            default:
                throw error::RuntimeError("Invalid state");
        }
    }

    mem::Offset Write(std::shared_ptr<mem::File>& file, mem::Offset offset) const override {
        switch (state_) {
            case ObjectState::kFree:
                file->Write<mem::Magic>(~magic_, offset);
                offset += sizeof(mem::Magic);
                return file->Write(std::get<mem::PageOffset>(meta_), offset);

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
            meta_ = file->Read<mem::PageOffset>(offset);
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
                        : std::string(", next_free: ")
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
    if (util::Is<ts::PrimitiveClass<P>>(data_class)) {                                           \
        data_ = ts::ReadNew<ts::Primitive<P>>(util::As<ts::PrimitiveClass<P>>(data_class), file, \
                                              offset);                                           \
    }
                DDB_PRIMITIVE_GENERATOR(DDB_CREATE_PRIMITIVE)
                else {
                    throw error::TypeError("Class can't be turned in Node");
                }
#undef CREATE_PRIMITIVE
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

    void Free(std::variant<mem::PageOffset, ObjectId> meta) {
        state_ = ObjectState::kFree;
        meta_ = meta;
    }

    [[nodiscard]] ObjectId Id() const {
        if (state_ != ObjectState::kValid) {
            throw error::BadArgument("Node has no id");
        }
        return std::get<ObjectId>(meta_);
    }

    [[nodiscard]] mem::PageOffset NextFree() const {
        if (state_ != ObjectState::kFree) {
            throw error::BadArgument("Node has no jumper");
        }
        return std::get<mem::PageOffset>(meta_);
    }

    [[nodiscard]] ObjectState State() const {
        return state_;
    }
};
}  // namespace db