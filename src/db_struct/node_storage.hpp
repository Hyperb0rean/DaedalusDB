#pragma once

#include "class_storage.hpp"

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
                std::shared_ptr<mem::PageAllocator>& alloc,
                std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : LOGGER(logger), nodes_class_(nodes_class), class_storage_(class_storage), alloc_(alloc) {

        DEBUG("Node storage initialized with class");
        data_page_list_ = mem::PageList(nodes_class->Name(), alloc_->GetFile(),
                                        GetHeader().GetNodeListSentinelOffset(), LOGGER);
    }
};

class ConstantSizeNodeStorage : public NodeStorage {
public:
    template <ts::ClassLike C>
    ConstantSizeNodeStorage(
        std::shared_ptr<C> nodes_class, std::shared_ptr<ClassStorage>& class_storage,
        std::shared_ptr<mem::PageAllocator>& alloc,
        std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : NodeStorage(nodes_class, class_storage, alloc, logger) {
    }

    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) void AddNode(std::shared_ptr<O>& node) {
    }
};

class VariableSizeNodeStorage : public NodeStorage {
public:
    template <ts::ClassLike C>
    VariableSizeNodeStorage(
        std::shared_ptr<C> nodes_class, std::shared_ptr<ClassStorage>& class_storage,
        std::shared_ptr<mem::PageAllocator>& alloc,
        std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : NodeStorage(nodes_class, class_storage, alloc, logger) {
    }

    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) void AddNode(std::shared_ptr<O>& node) {
    }
};

}  // namespace db
