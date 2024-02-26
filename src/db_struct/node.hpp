#pragma once

#include <variant>

#include "mem.hpp"
#include "new.hpp"

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

class Node {
private:
    mem::Magic magic_;
    // Stores for valid objects their ids and for free  next free inpage offset
    std::variant<mem::PageOffset, ts::ObjectId> meta_;
    ts::Object::Ptr data_;
    ObjectState state_;

public:
    using Ptr = util::Ptr<Node>;

    template <ts::ObjectLike O>
    Node(mem::Magic magic, ts::ObjectId id, util::Ptr<O> data)
        : magic_(magic), meta_(id), data_(data), state_(ObjectState::kValid) {
    }

    size_t Size() const {
        switch (state_) {
            case ObjectState::kFree: {
                return sizeof(mem::Magic) + sizeof(mem::PageOffset);
            }
            case ObjectState::kValid:
                return sizeof(mem::Magic) + sizeof(ts::ObjectId) + data_->Size();
            case ObjectState::kInvalid:
                throw error::BadArgument("Invalid object has no size");
            default:
                throw error::RuntimeError("Invalid state");
        }
    }

    mem::Offset Write(mem::File::Ptr& file, mem::Offset offset) const {
        switch (state_) {
            case ObjectState::kFree:
                file->Write<mem::Magic>(~magic_, offset);
                offset += sizeof(mem::Magic);
                return file->Write(std::get<mem::PageOffset>(meta_), offset);

            case ObjectState::kValid:
                file->Write<mem::Magic>(magic_, offset);
                offset += sizeof(mem::Magic);
                file->Write(std::get<ts::ObjectId>(meta_), offset);
                offset += sizeof(ts::ObjectId);
                return data_->Write(file, offset);
            case ObjectState::kInvalid:
                throw error::BadArgument("Trying to write invalid object");
            default:
                throw error::RuntimeError("Invalid state");
        }
    }
    void Read(mem::File::Ptr& file, mem::Offset offset) {
        auto magic = file->Read<mem::Magic>(offset);
        offset += sizeof(mem::Magic);

        if (magic == magic_) {
            state_ = ObjectState::kValid;
            meta_ = file->Read<ts::ObjectId>(offset);
            offset += sizeof(ts::ObjectId);
            data_->Read(file, offset);
        } else if (magic == ~magic_) {
            state_ = ObjectState::kFree;
            meta_ = file->Read<mem::PageOffset>(offset);
        } else {
            state_ = ObjectState::kInvalid;
        }
    }
    [[nodiscard]] std::string ToString() const {
        return std::string("NODE: ")
            .append("state: ")
            .append(ObjectStateToString(state_))
            .append(
                std::holds_alternative<ts::ObjectId>(meta_)
                    ? std::string(", id: ").append(std::to_string(std::get<ts::ObjectId>(meta_)))
                    : std::string(", next_free: ")
                          .append(std::to_string(std::get<mem::PageOffset>(meta_))))
            .append(", data: { ")
            .append(state_ == ObjectState::kValid ? data_->ToString() : ObjectStateToString(state_))
            .append(" } ");
    }

    Node(mem::Magic magic, ts::Class::Ptr data_class, mem::File::Ptr& file, mem::Offset offset)
        : magic_(magic) {

        auto read_magic = file->Read<mem::Magic>(offset);
        offset += sizeof(mem::Magic);

        if (read_magic == magic_) {
            state_ = ObjectState::kValid;
            meta_ = file->Read<ts::ObjectId>(offset);
            offset += sizeof(ts::ObjectId);

            if (util::Is<ts::StructClass>(data_class)) {
                data_ =
                    ts::ReadNew<ts::Struct>(util::As<ts::StructClass>(data_class), file, offset);
            } else if (util::Is<ts::StringClass>(data_class)) {
                data_ =
                    ts::ReadNew<ts::String>(util::As<ts::StringClass>(data_class), file, offset);
            } else if (util::Is<ts::RelationClass>(data_class)) {
                data_ = ts::ReadNew<ts::Relation>(util::As<ts::RelationClass>(data_class), file,
                                                  offset);
            } else {

#define DDB_CREATE_PRIMITIVE(P)                                                                  \
    else if (util::Is<ts::PrimitiveClass<P>>(data_class)) {                                      \
        data_ = ts::ReadNew<ts::Primitive<P>>(util::As<ts::PrimitiveClass<P>>(data_class), file, \
                                              offset);                                           \
    }

                if (false) {
                }
                DDB_PRIMITIVE_GENERATOR(DDB_CREATE_PRIMITIVE)
                else {
                    throw error::TypeError("Class can't be turned in Node");
                }
#undef CREATE_PRIMITIVE
            }
        } else if (read_magic == ~magic_) {
            state_ = ObjectState::kFree;
            meta_ = file->Read<mem::PageOffset>(offset);
        } else {
            state_ = ObjectState::kInvalid;
        }
    }

    void Free(mem::PageOffset meta) {
        state_ = ObjectState::kFree;
        meta_ = meta;
    }

    [[nodiscard]] ts::ObjectId Id() const {
        if (state_ != ObjectState::kValid) {
            throw error::BadArgument("Node has no id");
        }
        return std::get<ts::ObjectId>(meta_);
    }

    [[nodiscard]] mem::PageOffset NextFree() const {
        if (state_ != ObjectState::kFree) {
            throw error::BadArgument("Node has no next");
        }
        return std::get<mem::PageOffset>(meta_);
    }

    [[nodiscard]] ObjectState State() const {
        return state_;
    }

    template <ts::ObjectLike O>
    [[nodiscard]] util::Ptr<O> Data() const {
        switch (state_) {
            case ObjectState::kFree: {
                throw error::RuntimeError("Use after free");
            }
            case ObjectState::kValid:
                return util::As<O>(data_);
            case ObjectState::kInvalid:
                throw error::RuntimeError("Invalid node");
            default:
                throw error::RuntimeError("Invalid state");
        }
    }
};
}  // namespace db