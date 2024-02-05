#pragma once

#include "class_storage.hpp"

namespace db {

enum class OpenMode { kDefault, kRead, kWrite };
enum class PrintMode { kCache, kFile };

class Database {
    mem::Superblock superblock_;
    std::shared_ptr<mem::File> file_;
    std::shared_ptr<mem::PageAllocator> alloc_;
    std::shared_ptr<ClassStorage> class_storage_;
    std::shared_ptr<util::Logger> logger_;

    void InitializeSuperblock(OpenMode mode) {
        switch (mode) {
            case OpenMode::kRead: {
                logger_->Debug("OpenMode: Read");
                superblock_.ReadSuperblock(file_);
            } break;
            case OpenMode::kDefault: {
                logger_->Debug("OpenMode: Default");
                try {
                    superblock_.ReadSuperblock(file_);
                    break;
                } catch (const error::StructureError& e) {
                    logger_->Error("Can't open file in Read mode, rewriting..");
                } catch (const error::BadArgument& e) {
                    logger_->Error("Can't open file in Read mode, rewriting..");
                }
            }
            case OpenMode::kWrite: {
                logger_->Debug("OpenMode: Write");
                file_->Clear();
                superblock_.InitSuperblock(file_);
            } break;
        }
    }

public:
    Database(const std::shared_ptr<mem::File>& file, OpenMode mode = OpenMode::kDefault,
             std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : file_(file), logger_(logger) {

        InitializeSuperblock(mode);

        alloc_ = std::make_shared<mem::PageAllocator>(file_, logger_);
        logger_->Info("Alloc initialized");
        class_storage_ = std::make_shared<ClassStorage>(alloc_, logger_);
    }

    ~Database() {
        logger_->Info("Closing database");
    };

    template <ts::ClassLike C>
    void AddClass(std::shared_ptr<C>& new_class) {
        class_storage_->AddClass(new_class);
    }
};

}  // namespace db
