#pragma once

#include "class_storage.hpp"
#include "node.hpp"

namespace db {

class NodeStorage {
protected:
    DECLARE_LOGGER;
    std::shared_ptr<ts::Class> nodes_class_;
    std::shared_ptr<ClassStorage> class_storage_;
    std::shared_ptr<mem::PageAllocator> alloc_;
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
    NodeStorage(std::shared_ptr<C> nodes_class, std::shared_ptr<ClassStorage>& class_storage,
                std::shared_ptr<mem::PageAllocator>& alloc, DEFAULT_LOGGER(logger))
        : LOGGER(logger), nodes_class_(nodes_class), class_storage_(class_storage), alloc_(alloc) {

        data_page_list_ = mem::PageList(nodes_class->Name(), alloc_->GetFile(),
                                        GetHeader().GetNodeListSentinelOffset(), LOGGER);
    }
};

}  // namespace db
