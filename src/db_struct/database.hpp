#pragma once

#include <iostream>
#include <unordered_map>

#include "allocator.hpp"
#include "object.hpp"

namespace db {

enum class OpenMode { kDefault, kRead, kWrite };
enum class PrintMode { kCache, kFile };

class Database {
    mem::Superblock superblock_;
    mem::PageList class_list_;
    std::unordered_map<std::string, mem::ClassHeader> class_cache_;
    std::shared_ptr<mem::PageAllocator> alloc_;
    std::shared_ptr<mem::File> file_;
    std::shared_ptr<util::Logger> logger_;

    mem::Offset GetOffset(mem::PageIndex index, mem::PageOffset virt_offset) {
        return mem::kPagetableOffset + index * mem::kPageSize + virt_offset;
    }

    void InitializeClassCache() {
        logger_->Info("Initializing class cache..");

        class_cache_.clear();
        for (auto& class_it : class_list_) {
            ts::ClassObject class_object;
            class_object.Read(file_, GetOffset(class_it.index_, class_it.first_free_));
            logger_->Debug("Initialized: " + class_object.ToString());
            class_cache_.emplace(class_object.ToString(),
                                 mem::ClassHeader(class_it.index_).ReadClassHeader(file_));
        }
    }

    mem::ClassHeader InitializeClassHeader(mem::PageIndex index, size_t size) {
        return mem::ClassHeader(index)
            .ReadClassHeader(file_)
            .InitClassHeader(file_, size)
            .WriteClassHeader(file_);
    }

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

        logger->Debug("Class list sentinel offset: " +
                      std::to_string(mem::kClassListSentinelOffset));
        logger->Debug("Class list count: " +
                      std::to_string(file->Read<size_t>(mem::kClassListCount)));
        class_list_ = mem::PageList(file_, mem::kClassListSentinelOffset, logger_);
        logger_->Info("ClassList initialized");

        InitializeClassCache();
    }

    template <ts::ClassLike C>
    void AddClass(std::shared_ptr<C>& new_class) {
        ts::ClassObject class_object(new_class);

        if (class_cache_.contains(class_object.ToString())) {
            throw error::RuntimeError("Class already present in database");
        }

        if (class_object.GetSize() > mem::kPageSize - sizeof(mem::ClassHeader)) {
            throw error::NotImplemented("Too complex class");
        }

        logger_->Info("Adding class");
        logger_->Debug(class_object.ToString());

        auto header = InitializeClassHeader(alloc_->AllocatePage(), class_object.GetSize());
        logger_->Debug("Index: " + std::to_string(header.index_));
        class_list_.PushBack(header.index_);
        class_object.Write(file_, GetOffset(header.index_, header.first_free_));
        class_cache_.emplace(class_object.ToString(), header.index_);
    }

    // Rewrite to visitor
    void PrintAllClasses(PrintMode mode, std::ostream& cout = std::cout) {
        switch (mode) {
            case PrintMode::kCache: {
                for (auto& it : class_cache_) {
                    cout << "[" << it.second.index_ << "] : " << it.first << std::endl;
                }
            } break;
            case PrintMode::kFile: {
                for (auto& it : class_list_) {
                    ts::ClassObject class_object;
                    class_object.Read(file_, GetOffset(it.index_, it.first_free_));
                    cout << "[" << it.index_ << "] : " << class_object.ToString() << std::endl;
                }
            } break;
        }
    }

    ~Database() {
        logger_->Info("Closing database");
    };
};

}  // namespace db
