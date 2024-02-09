#pragma once

#include "class_storage.hpp"

namespace db {

class NodeStorage {
private:
    DECLARE_LOGGER;
    std::shared_ptr<ts::Class> nodes_class_;
    std::shared_ptr<ClassStorage> class_storage_;
    std::shared_ptr<mem::PageAllocator> alloc_;
    mem::PageList data_page_list_;

    mem::Page AllocatePage() {
        data_page_list_.PushBack(alloc_->AllocatePage());
        auto page =
            mem::Page(data_page_list_.Back()).ReadPage(alloc_->GetFile(), mem::kPagetableOffset);
        page.type_ = mem::PageType::kData;
        return page.WritePage(alloc_->GetFile(), mem::kPagetableOffset);
    }

    mem::Page GetBack() {
        if (data_page_list_.IsEmpty()) {
            return AllocatePage();
        } else {
            return mem::Page(data_page_list_.Back())
                .ReadPage(alloc_->GetFile(), mem::kPagetableOffset);
        }
    }

public:
    template <ts::ClassLike C>
    NodeStorage(std::shared_ptr<C> nodes_class, std::shared_ptr<ClassStorage>& class_storage,
                std::shared_ptr<mem::PageAllocator>& alloc,
                std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : LOGGER(logger), nodes_class_(nodes_class), class_storage_(class_storage), alloc_(alloc) {
        mem::PageIndex index;
        auto found = class_storage_->FindClass(nodes_class_);
        if (std::holds_alternative<std::monostate>(found)) {
            throw error::RuntimeError("No such class in class storage");
        } else if (std::holds_alternative<mem::PageIndex>(found)) {
            index = std::get<mem::PageIndex>(found);
        } else {
            index = std::get<ClassStorage::ClassCache::iterator>(found)->second;
        }
        DEBUG("Node storage initialized with class");
        data_page_list_ = mem::PageList(alloc_->GetFile(), mem::GetNodeListSentinel(index), LOGGER);
    }

    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) void AddNode(std::shared_ptr<O>& node) {

        if (node->Size() > mem::kPageSize - sizeof(mem::Page)) {
            throw error::NotImplemented("Too big object)");
        }

        auto back = GetBack();
        if (node->Size() + mem::GetOffset(back.index_, back.first_free_) >
            mem::GetOffset(back.index_ + 1, 0)) {
            back = AllocatePage();
        }

        INFO("Writing Object: ", node->ToString());
        node->Write(alloc_->GetFile(), mem::GetOffset(back.index_, back.first_free_));
    }
};
}  // namespace db
