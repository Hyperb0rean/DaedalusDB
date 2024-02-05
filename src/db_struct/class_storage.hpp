#pragma once

#include <allocator.hpp>
#include <object.hpp>
#include <unordered_map>

class ClassStorage {

    std::shared_ptr<mem::PageAllocator> alloc_;
    std::shared_ptr<util::Logger> logger_;
    mem::PageList class_list_;
    std::unordered_map<std::string, mem::ClassHeader> class_cache_;

    void InitializeClassCache() {
        logger_->Info("Initializing class cache..");

        class_cache_.clear();
        for (auto& class_it : class_list_) {
            ts::ClassObject class_object;
            class_object.Read(alloc_->GetFile(),
                              mem::GetOffset(class_it.index_, class_it.first_free_));
            logger_->Debug("Initialized: " + class_object.ToString());
            class_cache_.emplace(
                class_object.ToString(),
                mem::ClassHeader(class_it.index_).ReadClassHeader(alloc_->GetFile()));
        }
    }

    mem::ClassHeader InitializeClassHeader(mem::PageIndex index, size_t size) {
        return mem::ClassHeader(index)
            .ReadClassHeader(alloc_->GetFile())
            .InitClassHeader(alloc_->GetFile(), size)
            .WriteClassHeader(alloc_->GetFile());
    }

public:
    ClassStorage(std::shared_ptr<mem::PageAllocator>& alloc,
                 std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : alloc_(alloc), logger_(logger) {

        logger_->Debug("Class list sentinel offset: " +
                       std::to_string(mem::kClassListSentinelOffset));
        logger_->Debug("Class list count: " +
                       std::to_string(alloc_->GetFile()->Read<size_t>(mem::kClassListCount)));

        class_list_ = mem::PageList(alloc_->GetFile(), mem::kClassListSentinelOffset, logger_);

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
        class_object.Write(alloc_->GetFile(), mem::GetOffset(header.index_, header.first_free_));
        class_cache_.emplace(class_object.ToString(), header.index_);
    }
};