#pragma once

#include "class_storage.hpp"

namespace db {

enum class OpenMode { kDefault, kRead, kWrite };

class Database {

    DECLARE_LOGGER;
    mem::Superblock superblock_;
    std::shared_ptr<mem::File> file_;
    std::shared_ptr<mem::PageAllocator> alloc_;
    std::shared_ptr<ClassStorage> class_storage_;

    void InitializeSuperblock(OpenMode mode) {
        switch (mode) {
            case OpenMode::kRead: {
                DEBUG("OpenMode: Read");
                superblock_.ReadSuperblock(file_);
            } break;
            case OpenMode::kDefault: {
                DEBUG("OpenMode: Default");
                try {
                    superblock_.ReadSuperblock(file_);
                    break;
                } catch (const error::StructureError& e) {
                    ERROR("Can't open file in Read mode, rewriting..");
                } catch (const error::BadArgument& e) {
                    ERROR("Can't open file in Read mode, rewriting..");
                }
            }
            case OpenMode::kWrite: {
                DEBUG("OpenMode: Write");
                file_->Clear();
                superblock_.InitSuperblock(file_);
            } break;
        }
    }

public:
    Database(const std::shared_ptr<mem::File>& file, OpenMode mode = OpenMode::kDefault,
             std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : LOGGER(logger), file_(file) {

        InitializeSuperblock(mode);

        alloc_ = std::make_shared<mem::PageAllocator>(file_, LOGGER);
        INFO("Allocator initialized");
        class_storage_ = std::make_shared<ClassStorage>(alloc_, LOGGER);
    }

    ~Database() {
        INFO("Closing database");
    };

    template <ts::ClassLike C>
    void AddClass(std::shared_ptr<C>& new_class) {
        class_storage_->AddClass(new_class);
    }
};

}  // namespace db
