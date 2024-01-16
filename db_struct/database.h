#pragma once

#include <unordered_map>

#include "../mem/allocator.h"
#include "types.h"

namespace db {

enum class Mode { kDefault, kOpen, kWrite };

class Database {
    std::shared_ptr<mem::Superblock> superblock_;
    std::shared_ptr<mem::File> file_;
    std::vector<std::shared_ptr<types::Type>> types_;

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
                    superblock_->WriteSuperblock(file_);
                }

            } break;
        }
    }
    void AddNode(types::Object node) {
        (void)node;
    }

    template <typename ObjectType>
    [[nodiscard]] std::shared_ptr<ObjectType> GetNode(
        size_t index) requires std::derived_from<ObjectType, types::Object> {
    }

    ~Database() {
    }
};

}  // namespace db
