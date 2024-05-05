#pragma once

#include "allocator.hpp"
#include "class_storage.hpp"
#include "logger.hpp"

namespace db {

class NodeStorage {
protected:
    DECLARE_LOGGER;
    ts::Class::Ptr nodes_class_;
    ClassStorage::Ptr class_storage_;
    mem::PageAllocator::Ptr alloc_;
    mem::PageList data_page_list_;

    auto AllocatePage() -> mem::Page {
        data_page_list_.PushBack(alloc_->AllocatePage());
        DEBUG(mem::Page(data_page_list_.Back()));
        auto page = ReadPage(mem::Page(data_page_list_.Back()), alloc_->GetFile());
        page.type_ = mem::PageType::kData;
        DEBUG(page);
        return WritePage(page, alloc_->GetFile());
    }

    auto FreePage(mem::PageIndex index) -> void {
        data_page_list_.Unlink(index);
        alloc_->FreePage(index);
    }

    auto GetBack() -> mem::Page {
        if (data_page_list_.IsEmpty()) {
            // TODO: probably should throw
            return AllocatePage();
        } else {
            return ReadPage(mem::Page(data_page_list_.Back()), alloc_->GetFile());
        }
    }

    auto GetFront() -> mem::Page {
        if (data_page_list_.IsEmpty()) {
            throw error::RuntimeError("No front");
        } else {
            return ReadPage(mem::Page(data_page_list_.Front()), alloc_->GetFile());
        }
    }

    auto GetHeader() -> mem::ClassHeader {
        auto index = class_storage_->FindClass(nodes_class_);
        if (!index.has_value()) {
            throw error::RuntimeError("No such class in class storage");
        }
        return mem::ClassHeader(index.value()).ReadClassHeader(alloc_->GetFile());
    }

public:
    template <ts::ClassLike C>
    NodeStorage(const util::Ptr<C>& nodes_class, ClassStorage::Ptr& class_storage,
                mem::PageAllocator::Ptr& alloc, DEFAULT_LOGGER(logger))
        : LOGGER(logger), nodes_class_(nodes_class), class_storage_(class_storage), alloc_(alloc) {

        data_page_list_ = mem::PageList(nodes_class->Name(), alloc_->GetFile(),
                                        GetHeader().GetNodeListSentinelOffset(), LOGGER);
    }

    auto Drop() -> void {
        std::vector<mem::PageIndex> indicies;
        for (auto& page : data_page_list_) {
            DEBUG("Freeing page: ", page);
            indicies.push_back(page.index_);
        }
        for (auto index : indicies) {
            FreePage(index);
        }
    }
};

}  // namespace db
