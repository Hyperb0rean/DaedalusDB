#pragma once

#include <string>
#include <unordered_map>

#include "allocator.hpp"
#include "class_object.hpp"

namespace db {

enum class DataMode { kCache, kFile };

class ClassStorage {
private:
    DECLARE_LOGGER;
    mem::PageAllocator::Ptr alloc_;
    mem::PageList class_list_;

    using ClassCache = std::unordered_map<std::string, mem::PageIndex>;
    ClassCache class_cache_;

    auto GetSerializedClass(mem::PageIndex index) const -> std::string {
        auto header = mem::ClassHeader(index).ReadClassHeader(alloc_->GetFile());
        ts::ClassObject class_object;
        class_object.Read(alloc_->GetFile(), mem::GetOffset(header.index_, header.free_offset_));
        return class_object.ToString();
    }

    auto InitializeClassCache() -> void {
        INFO("Initializing class cache..");

        class_cache_.clear();

        for (auto& class_it : class_list_) {
            auto serialized = GetSerializedClass(class_it.index_);
            DEBUG("Initialized:", serialized);
            class_cache_.emplace(serialized, class_it.index_);
        }
    }

    auto EraseFromCache(mem::PageIndex index) -> void {
        for (auto it = class_cache_.begin(); it != class_cache_.end();) {
            if (it->second == index) {
                it = class_cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    template <ts::ClassLike C>
    auto MakeClassHolder(const util::Ptr<C>& new_class) -> ts::ClassObject::Ptr {
        return util::MakePtr<ts::ClassObject>(new_class);
    }

    auto InitializeClassHeader(mem::PageIndex index, ts::ClassObject::Ptr& class_object)
        -> mem::ClassHeader {
        return mem::ClassHeader(index)
            .ReadClassHeader(alloc_->GetFile())
            .InitClassHeader(alloc_->GetFile(), class_object->Size())
            .WriteMagic(alloc_->GetFile(), rand());
    }

public:
    using Ptr = util::Ptr<ClassStorage>;

    ClassStorage(mem::PageAllocator::Ptr& alloc, DEFAULT_LOGGER(logger))
        : LOGGER(logger), alloc_(alloc) {

        DEBUG("Class list sentinel offset:", mem::kClassListSentinelOffset);
        DEBUG("Class list count:", alloc_->GetFile()->Read<size_t>(
                                       mem::GetCountFromSentinel(mem::kClassListSentinelOffset)));

        class_list_ =
            mem::PageList("Class_List", alloc_->GetFile(), mem::kClassListSentinelOffset, LOGGER);

        INFO("ClassList initialized");

        InitializeClassCache();
    }

    template <ts::ClassLike C>
    auto FindClass(util::Ptr<C> new_class, DataMode mode = DataMode::kCache)
        -> std::optional<mem::PageIndex> {
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
    auto AddClass(const util::Ptr<C>& new_class) -> void {
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
                                    mem::GetOffset(header.index_, header.free_offset_));
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
    auto RemoveClass(const util::Ptr<C>& new_class) -> void {
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

        class_list_.Unlink(index.value());
        alloc_->FreePage(index.value());
    }

    template <typename F>
    requires std::invocable<F, mem::ClassHeader>
    auto VisitClasses(F functor) -> void {
        for (auto& class_header : class_list_) {
            functor(mem::ClassHeader(class_header.index_).ReadClassHeader(alloc_->GetFile()));
        }
    }

    template <typename Functor>
    requires std::invocable<Functor, ts::Class::Ptr>
    auto VisitClasses(Functor functor) -> void {
        for (auto& class_header : class_list_) {
            ts::ClassObject class_object;
            class_object.Read(alloc_->GetFile(),
                              mem::GetOffset(class_header.index_, class_header.free_offset_));
            functor(class_object.GetClass());
        }
    }
};
}  // namespace db