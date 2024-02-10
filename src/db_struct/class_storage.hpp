#pragma once

#include <unordered_map>

#include "allocator.hpp"
#include "object.hpp"

namespace db {

enum class DataMode { kCache, kFile };

class ClassStorage {
private:
    DECLARE_LOGGER;
    std::shared_ptr<mem::PageAllocator> alloc_;
    mem::PageList class_list_;

    using ClassCache = std::unordered_map<std::string, mem::PageIndex>;
    ClassCache class_cache_;

    std::string GetSerializedClass(mem::PageIndex index) const {
        auto header = mem::ClassHeader(index).ReadClassHeader(alloc_->GetFile());
        ts::ClassObject class_object;
        class_object.Read(alloc_->GetFile(), mem::GetOffset(header.index_, header.first_free_));
        return class_object.ToString();
    }

    void InitializeClassCache() {
        INFO("Initializing class cache..");

        class_cache_.clear();

        for (auto& class_it : class_list_) {
            auto serialized = GetSerializedClass(class_it.index_);
            DEBUG("Initialized:", serialized);
            class_cache_.emplace(serialized, class_it.index_);
        }
    }

    void EraseFromCache(mem::PageIndex index) {
        for (auto it = class_cache_.begin(); it != class_cache_.end();) {
            if (it->second == index) {
                it = class_cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    template <ts::ClassLike C>
    std::shared_ptr<ts::ClassObject> MakeClassHolder(std::shared_ptr<C>& new_class) {
        return std::make_shared<ts::ClassObject>(new_class);
    }

    mem::ClassHeader InitializeMagic(mem::ClassHeader header,
                                     std::shared_ptr<ts::ClassObject>& class_object) {
        DEBUG("Initializing fundamental class constant");
        return header;
    }

    mem::ClassHeader InitializeClassHeader(mem::PageIndex index,
                                           std::shared_ptr<ts::ClassObject>& class_object) {
        return InitializeMagic(mem::ClassHeader(index)
                                   .ReadClassHeader(alloc_->GetFile())
                                   .InitClassHeader(alloc_->GetFile(), class_object->Size()),
                               class_object)
            .WriteClassHeader(alloc_->GetFile());
    }

public:
    ClassStorage(std::shared_ptr<mem::PageAllocator>& alloc,
                 std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : LOGGER(logger), alloc_(alloc) {

        DEBUG("Class list sentinel offset:", mem::kClassListSentinelOffset);
        DEBUG("Class list count:", alloc_->GetFile()->Read<size_t>(mem::kClassListCount));

        class_list_ =
            mem::PageList("Class_List", alloc_->GetFile(), mem::kClassListSentinelOffset, LOGGER);

        INFO("ClassList initialized");

        InitializeClassCache();
    }

    template <ts::ClassLike C>
    std::optional<mem::PageIndex> FindClass(std::shared_ptr<C> new_class,
                                            DataMode mode = DataMode::kCache) {
        auto class_object = MakeClassHolder(new_class);
        auto serialized = class_object->ToString();
        if (class_cache_.contains(serialized)) {
            switch (mode) {
                case DataMode::kCache:
                    return class_cache_[serialized];
                case DataMode::kFile: {
                    if (serialized == GetSerializedClass(class_cache_[serialized])) {
                        return class_cache_[serialized];
                    }
                } break;
            }
        }

        if (mode == DataMode::kFile) {
            for (auto& page : class_list_) {
                if (GetSerializedClass(page.index_) == serialized) {
                    return page.index_;
                }
            }
        }

        return std::nullopt;
    }

    template <ts::ClassLike C>
    void AddClass(std::shared_ptr<C>& new_class) {
        INFO("Adding new class..");

        auto class_object = MakeClassHolder(new_class);

        if (class_object->Size() > mem::kPageSize - sizeof(mem::ClassHeader)) {
            throw error::NotImplemented("Too complex class");
        }

        auto index = FindClass(new_class, DataMode::kFile);
        auto cache_index = FindClass(new_class, DataMode::kCache);

        if (!cache_index.has_value()) {
            DEBUG(class_object->ToString());
            if (!index.has_value()) {
                auto header = InitializeClassHeader(alloc_->AllocatePage(), class_object);
                DEBUG("Index: ", header.index_);

                class_list_.PushBack(header.index_);
                class_object->Write(alloc_->GetFile(),
                                    mem::GetOffset(header.index_, header.first_free_));
                class_cache_.emplace(class_object->ToString(), header.index_);
            } else {
                INFO("Adding class to cache");
                class_cache_.emplace(class_object->ToString(), index.value());
            }

        } else {
            if (!index.has_value()) {
                WARN("Cache miss");
                EraseFromCache(cache_index.value());

            } else {
                ERROR(class_object->ToString());
                ERROR("Class already present");
            }
        }
    }

    template <ts::ClassLike C>
    void RemoveClass(std::shared_ptr<C>& new_class) {
        INFO("Removing class..");

        auto index = FindClass(new_class, DataMode::kFile);
        auto cache_index = FindClass(new_class, DataMode::kCache);

        if (cache_index.has_value()) {
            EraseFromCache(cache_index.value());
            if (!index.has_value()) {
                WARN("Removing from cache");
                return;
            }
        }

        if (!index.has_value()) {
            ERROR("Class not present");
            return;
        }

        // TODO: REMOVE ALL NODES OF CLASS
        class_list_.Unlink(index.value());
        alloc_->FreePage(index.value());
    }

    template <typename F>
    requires std::invocable<F, mem::ClassHeader>
    void VisitClasses(F functor) {
        for (auto& class_header : class_list_) {
            functor(mem::ClassHeader(class_header.index_).ReadClassHeader(alloc_->GetFile()));
        }
    }

    template <typename F>
    requires std::invocable<F, ts::ClassObject>
    void VisitClasses(F functor) {
        for (auto& class_header : class_list_) {
            ts::ClassObject class_object;
            class_object.Read(alloc_->GetFile(),
                              mem::GetOffset(class_header.index_, class_header.first_free_));
            functor(class_object);
        }
    }
};
}  // namespace db