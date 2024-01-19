#pragma once

#include <unordered_map>

#include "object.hpp"
#include "pagelist.hpp"

namespace db {

enum class Mode { kDefault, kOpen, kWrite };

class Database {
    std::shared_ptr<mem::Superblock> superblock_;
    std::shared_ptr<mem::File> file_;

public:
    Database(const std::shared_ptr<mem::File>& file, Mode mode = Mode::kDefault) : file_(file) {
        switch (mode) {
            case Mode::kOpen: {
                superblock_->ReadSuperblock(file_);
            } break;
            case Mode::kWrite: {
                superblock_->WriteSuperblock(file_);
            } break;
            case Mode::kDefault: {
                try {
                    superblock_->ReadSuperblock(file_);
                } catch (const error::StructureError& e) {
                    std::cerr << "Can't open file in Read mode, rewriting..";
                    superblock_->WriteSuperblock(file_);
                }

            } break;
        }
    }
    void AddNode(ts::Object node) {
        (void)node;
    }

    template <typename ObjectType>
    [[nodiscard]] std::shared_ptr<ObjectType> GetNode(
        size_t index) requires std::derived_from<ObjectType, ts::Object> {
    }

    ~Database() = default;
};

}  // namespace db
