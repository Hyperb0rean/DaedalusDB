#pragma once

#include <unordered_map>
#include <variant>

#include "allocator.hpp"
#include "object.hpp"

namespace db {

enum class DataMode { kCache, kFile };

class ClassStorage {

    std::shared_ptr<mem::PageAllocator> alloc_;
    std::shared_ptr<util::Logger> logger_;
    mem::PageList class_list_;

    using ClassCache = std::unordered_map<std::string, mem::PageIndex>;
    ClassCache class_cache_;

    using ClassIndex = std::variant<ClassCache::iterator, mem::PageIndex, std::monostate>;

    std::string GetSerializedClass(mem::PageIndex index) const {
        auto header = mem::ClassHeader(index).ReadClassHeader(alloc_->GetFile());
        ts::ClassObject class_object;
        class_object.Read(alloc_->GetFile(), mem::GetOffset(header.index_, header.first_free_));
        return class_object.ToString();
    }

    bool CheckCoherency(const ClassCache::iterator& it) const {
        return it->first == GetSerializedClass(it->second);
    }

    void InitializeClassCache() {
        INFO("Initializing class cache..");

        class_cache_.clear();

        for (auto& class_it : class_list_) {
            auto serialized = GetSerializedClass(class_it.index_);
            DEBUG("Initialized: " + serialized);
            class_cache_.emplace(serialized, class_it.index_);
        }
    }

    template <ts::ClassLike C>
    std::shared_ptr<ts::ClassObject> MakeClassHolder(std::shared_ptr<C>& new_class) {
        return std::make_shared<ts::ClassObject>(new_class);
    }

    mem::ClassHeader InitializeClassHeader(mem::PageIndex index, size_t size) {
        return mem::ClassHeader(index)
            .ReadClassHeader(alloc_->GetFile())
            .InitClassHeader(alloc_->GetFile(), size)
            .WriteClassHeader(alloc_->GetFile());
    }

    ClassIndex FindClass(std::shared_ptr<ts::ClassObject> class_object,
                         DataMode mode = DataMode::kCache) {
        auto class_it = class_cache_.begin();
        auto serialized = class_object->ToString();
        for (; class_it != class_cache_.end(); ++class_it) {
            if (class_it->first == serialized) {
                switch (mode) {
                    case DataMode::kCache:
                        return class_it;
                    case DataMode::kFile: {
                        if (CheckCoherency(class_it)) {
                            return class_it;
                        }
                    } break;
                }
            }
        }

        if (mode == DataMode::kFile) {
            for (auto& page : class_list_) {
                if (GetSerializedClass(page.index_) == serialized) {
                    return page.index_;
                }
            }
        }

        return std::monostate{};
    }

public:
    ClassStorage(std::shared_ptr<mem::PageAllocator>& alloc,
                 std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : alloc_(alloc), logger_(logger) {

        DEBUG("Class list sentinel offset: " + std::to_string(mem::kClassListSentinelOffset));
        DEBUG("Class list count: " +
              std::to_string(alloc_->GetFile()->Read<size_t>(mem::kClassListCount)));

        class_list_ = mem::PageList(alloc_->GetFile(), mem::kClassListSentinelOffset, logger_);

        INFO("ClassList initialized");

        InitializeClassCache();
    }

    template <ts::ClassLike C>
    void AddClass(std::shared_ptr<C>& new_class) {
        auto class_object = MakeClassHolder(new_class);

        if (class_object->GetSize() > mem::kPageSize - sizeof(mem::ClassHeader)) {
            throw error::NotImplemented("Too complex class");
        }
        if (std::holds_alternative<std::monostate>(FindClass(class_object, DataMode::kCache))) {

            auto found = FindClass(class_object, DataMode::kFile);

            if (std::holds_alternative<std::monostate>(found)) {
                INFO("Adding new class");
                DEBUG(class_object->ToString());

                auto header =
                    InitializeClassHeader(alloc_->AllocatePage(), class_object->GetSize());
                DEBUG("Index: " + std::to_string(header.index_));

                class_list_.PushBack(header.index_);
                class_object->Write(alloc_->GetFile(),
                                    mem::GetOffset(header.index_, header.first_free_));
                class_cache_.emplace(class_object->ToString(), header.index_);
            } else if (std::holds_alternative<mem::PageIndex>(found)) {
                INFO("Adding class to cache");
                DEBUG(class_object->ToString());
                class_cache_.emplace(class_object->ToString(), std::get<mem::PageIndex>(found));
            } else {
                throw error::RuntimeError("Cache miss");
            }

        } else {
            ERROR(class_object->ToString());
            throw error::RuntimeError("Class already present");
        }
    }

    template <ts::ClassLike C>
    void RemoveClass(std::shared_ptr<C>& new_class) {
        auto class_object = MakeClassHolder(new_class);
    }

    void VisitClasses() {
    }
};
}  // namespace db