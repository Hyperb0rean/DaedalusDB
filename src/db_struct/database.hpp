#pragma once

#include <iostream>
#include <unordered_map>

#include "object.hpp"
#include "pagelist.hpp"

namespace db {

enum class OpenMode { kDefault, kRead, kWrite };
enum class PrintMode { kCache, kFile };

class Database {
    mem::Superblock superblock_;
    mem::PageList free_list_;
    mem::PageList class_list_;
    std::unordered_map<std::string, mem::PageIndex> class_map_;
    std::shared_ptr<mem::PageAllocator> alloc_;
    std::shared_ptr<mem::File> file_;
    std::shared_ptr<util::Logger> logger_;

    mem::Offset GetOffset(mem::PageIndex index, mem::PageOffset virt_offset) {
        return superblock_.cr3_ + index * mem::kPageSize + virt_offset;
    }

    void InitializeTypeMap() {
        class_map_.clear();
        for (auto& class_it : class_list_) {
            ts::ClassObject class_object;
            class_object.Read(file_, GetOffset(class_it.index_, class_it.first_free_));
            class_map_.emplace(class_object.ToString(), class_it.index_);
        }
    }

    mem::PageIndex AllocatePage() {
        if (free_list_.IsEmpty()) {
            return alloc_->AllocatePage();
        }
        auto index = free_list_.Back();
        free_list_.PopBack();
        return index;
    }

    void FreePage(mem::PageIndex index) {
        if (free_list_.IteratorTo(index)->type_ == mem::PageType::kFree) {
            throw error::RuntimeError("Double free");
        }
        free_list_.PushBack(index);

        // Rearranding list;
    }

public:
    Database(const std::shared_ptr<mem::File>& file, OpenMode mode = OpenMode::kDefault,
             std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : file_(file), logger_(logger) {

        switch (mode) {
            case OpenMode::kRead: {
                logger_->Verbose("OpenMode: Read");
                superblock_.ReadSuperblock(file_);
            } break;
            case OpenMode::kWrite: {
                logger_->Verbose("OpenMode: Write");
                file->Clear();
                superblock_.InitSuperblock(file_);
            } break;
            case OpenMode::kDefault: {
                logger_->Verbose("OpenMode: Default");
                try {
                    superblock_.ReadSuperblock(file_);
                } catch (const error::StructureError& e) {
                    logger_->Error("Can't open file in Read mode, rewriting..");
                    superblock_.InitSuperblock(file_);
                } catch (const error::BadArgument& e) {
                    logger_->Error("Can't open file in Read mode, rewriting..");
                    superblock_.InitSuperblock(file_);
                }

            } break;
        }

        alloc_ = std::make_shared<mem::PageAllocator>(file_, superblock_.cr3_, logger_);
        logger_->Log("Alloc initialized");
        free_list_ = mem::PageList(alloc_, mem::kFreeListSentinelOffset, logger_);
        logger_->Log("FreeList initialized");
        class_list_ = mem::PageList(alloc_, mem::kClassListSentinelOffset, logger_);
        logger_->Log("ClassList initialized");

        InitializeTypeMap();
    }

    template <ts::ClassLike C>
    void AddClass(std::shared_ptr<C>& new_class) {
        ts::ClassObject class_object(new_class);

        if (class_map_.contains(class_object.ToString())) {
            throw error::RuntimeError("Class already present in database");
        }

        if (class_object.GetSize() > mem::kPageSize - sizeof(mem::ClassHeader)) {
            throw error::NotImplemented("Too complex class");
        }

        mem::PageIndex index = AllocatePage();
        class_list_.PushBack(index);

        mem::ClassHeader header(index);
        header.ReadClassHeader(superblock_.cr3_, file_);
        header.InitClassHeader(superblock_.cr3_, file_);
        header.actual_size_ = class_object.GetSize();
        header.WriteClassHeader(superblock_.cr3_, file_);

        class_object.Write(file_, GetOffset(header.index_, header.first_free_));
        class_map_.emplace(class_object.ToString(), header.index_);
    }

    void PrintAllClasses(PrintMode mode, std::ostream& cout = std::cout) {
        switch (mode) {
            case PrintMode::kCache: {
                for (auto& it : class_map_) {
                    cout << "[" << it.second << "] : " << it.first << std::endl;
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
        superblock_.WriteSuperblock(file_);
    };
};

}  // namespace db
