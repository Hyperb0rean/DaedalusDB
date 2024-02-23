#pragma once

#include "../mem/allocator.hpp"
#include "class_storage.hpp"

namespace db {

class NodeStorage {
protected:
    DECLARE_LOGGER;
    ts::Class::Ptr nodes_class_;
    ClassStorage::Ptr class_storage_;
    mem::PageAllocator::Ptr alloc_;
    mem::PageList data_page_list_;

    mem::Page AllocatePage() {
        data_page_list_.PushBack(alloc_->AllocatePage());
        auto page = ReadPage(mem::Page(data_page_list_.Back()), alloc_->GetFile());
        page.type_ = mem::PageType::kData;
        return WritePage(page, alloc_->GetFile());
    }

    void FreePage(mem::PageIndex index) {
        data_page_list_.Unlink(index);
        alloc_->FreePage(index);
    }

    mem::Page GetBack() {
        if (data_page_list_.IsEmpty()) {
            return AllocatePage();
        } else {
            return ReadPage(mem::Page(data_page_list_.Back()), alloc_->GetFile());
        }
    }

    mem::Page GetFront() {
        if (data_page_list_.IsEmpty()) {
            throw error::RuntimeError("No front");
        } else {
            return ReadPage(mem::Page(data_page_list_.Front()), alloc_->GetFile());
        }
    }

    mem::ClassHeader GetHeader() {
        auto index = class_storage_->FindClass(nodes_class_);
        if (!index.has_value()) {
            throw error::RuntimeError("No such class in class storage");
        }
        return mem::ClassHeader(index.value()).ReadClassHeader(alloc_->GetFile());
    }

public:
    template <ts::ClassLike C>
    NodeStorage(util::Ptr<C> nodes_class, ClassStorage::Ptr& class_storage,
                mem::PageAllocator::Ptr& alloc, DEFAULT_LOGGER(logger))
        : LOGGER(logger), nodes_class_(nodes_class), class_storage_(class_storage), alloc_(alloc) {

        data_page_list_ = mem::PageList(nodes_class->Name(), alloc_->GetFile(),
                                        GetHeader().GetNodeListSentinelOffset(), LOGGER);
    }
};

}  // namespace db
