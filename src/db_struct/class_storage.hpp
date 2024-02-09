#pragma once

#include <unordered_map>
#include <variant>

#include "allocator.hpp"
#include "object.hpp"

namespace db {

enum class DataMode { kCache, kFile };

class ClassStorage {
public:
    using ClassCache = std::unordered_map<std::string, mem::PageIndex>;

private:
    DECLARE_LOGGER;
    std::shared_ptr<mem::PageAllocator> alloc_;
    mem::PageList class_list_;

    ClassCache class_cache_;

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
            DEBUG("Initialized:", serialized);
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

public:
    ClassStorage(std::shared_ptr<mem::PageAllocator>& alloc,
                 std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : LOGGER(logger), alloc_(alloc) {

        DEBUG("Class list sentinel offset:", mem::kClassListSentinelOffset);
        DEBUG("Class list count:", alloc_->GetFile()->Read<size_t>(mem::kClassListCount));

        class_list_ = mem::PageList(alloc_->GetFile(), mem::kClassListSentinelOffset, LOGGER);

        INFO("ClassList initialized");

        InitializeClassCache();
    }

    using ClassIndex = std::variant<ClassCache::iterator, mem::PageIndex, std::monostate>;

    template <ts::ClassLike C>
    ClassIndex FindClass(std::shared_ptr<C> new_class, DataMode mode = DataMode::kCache) {
        auto class_object = MakeClassHolder(new_class);

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

    template <ts::ClassLike C>
    void AddClass(std::shared_ptr<C>& new_class) {
        INFO("Adding new class..");

        auto class_object = MakeClassHolder(new_class);

        if (class_object->GetSize() > mem::kPageSize - sizeof(mem::ClassHeader)) {
            throw error::NotImplemented("Too complex class");
        }
        if (std::holds_alternative<std::monostate>(FindClass(new_class, DataMode::kCache))) {

            auto found = FindClass(new_class, DataMode::kFile);

            if (std::holds_alternative<std::monostate>(found)) {
                DEBUG(class_object->ToString());

                auto header =
                    InitializeClassHeader(alloc_->AllocatePage(), class_object->GetSize());
                DEBUG("Index: ", header.index_);

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
            ERROR("Class already present");
            return;
        }
    }

    template <ts::ClassLike C>
    void RemoveClass(std::shared_ptr<C>& new_class) {
        INFO("Removing class..");

        auto found = FindClass(new_class, DataMode::kFile);
        auto cache_found = FindClass(new_class, DataMode::kCache);
        mem::PageIndex index;

        if (!std::holds_alternative<std::monostate>(cache_found)) {
            index = std::get<ClassCache::iterator>(cache_found)->second;
            class_cache_.erase(std::get<ClassCache::iterator>(cache_found));
            if (std::holds_alternative<std::monostate>(found)) {
                DEBUG("Removing from cache");
                return;
            }
        }

        if (std::holds_alternative<std::monostate>(found)) {
            ERROR("Class not present");
            return;
        } else if (std::holds_alternative<mem::PageIndex>(found)) {
            index = std::get<mem::PageIndex>(found);
        }

        // TODO: REMOVE ALL NODES OF CLASS
        class_list_.Unlink(index);
        alloc_->FreePage(index);
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