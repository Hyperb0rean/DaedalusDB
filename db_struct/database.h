#pragma once

#include <unordered_map>

#include "types.h"

namespace db {

class Database {
    const int64_t kMagic = 0xDEADBEEF;
    std::unique_ptr<mem::File> file_;
    std::vector<std::shared_ptr<types::Type>> types_;

    void CheckConsistency() {
        auto magic = file_->Read<int64_t>();
        if (magic != kMagic) {
            throw error::StructureError("Can't open database from this file: " +
                                        file_->GetFilename());
        }
    }

    [[nodiscard]] mem::Offset ReadSuperblock() const {
        return 0;
    }

    [[nodiscard]] mem::Offset InitSuperblock() {
        return 0;
    }

public:
    Database(const std::vector<std::shared_ptr<types::Type>>& types,
             std::unique_ptr<mem::File> file)
        : file_(std::move(file)) {
        try {
            CheckConsistency();
        } catch (const error::StructureError& e) {
        }
    }
    void AddNode(types::Object node) {
        (void)node;
    }
    ~Database() {
    }
};

}  // namespace db
