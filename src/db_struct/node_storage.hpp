#pragma once

#include "class_storage.hpp"
#include "metaobject.hpp"

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

    mem::Page GetBack() {
        if (data_page_list_.IsEmpty()) {
            return AllocatePage();
        } else {
            return ReadPage(mem::Page(data_page_list_.Back()), alloc_->GetFile());
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

class ConstantSizeNodeStorage : public NodeStorage {

    class ObjectIterator {

        std::shared_ptr<MetaObject> curr_;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = MetaObject;
        using difference_type = size_t;
        using pointer = std::shared_ptr<MetaObject>;
        using reference = MetaObject&;

        ObjectIterator() {
        }
        ObjectIterator& operator++() {
            return *this;
        }
        ObjectIterator operator++(int) {
            auto temp = *this;
            return temp;
        }

        ObjectIterator& operator--() {
            return *this;
        }
        ObjectIterator operator--(int) {
            auto temp = *this;
            return temp;
        }

        reference operator*() {
            return *curr_;
        }
        pointer operator->() {
            return curr_;
        }

        bool operator==(const ObjectIterator& other) const {
            return curr_->Id() == other.curr_->Id();
        }
        bool operator!=(const ObjectIterator& other) const {
            return !(*this == other);
        }
    };

public:
    template <ts::ClassLike C>
    ConstantSizeNodeStorage(std::shared_ptr<C> nodes_class,
                            std::shared_ptr<ClassStorage>& class_storage,
                            std::shared_ptr<mem::PageAllocator>& alloc, DEFAULT_LOGGER(logger))
        : NodeStorage(nodes_class, class_storage, alloc, logger) {
        DEBUG("Constant Node storage initialized with class: ",
              ts::ClassObject(nodes_class).ToString());
    }

    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) void AddNode(std::shared_ptr<O>& node) {
    }
};

class VariableSizeNodeStorage : public NodeStorage {
public:
    template <ts::ClassLike C>
    VariableSizeNodeStorage(std::shared_ptr<C> nodes_class,
                            std::shared_ptr<ClassStorage>& class_storage,
                            std::shared_ptr<mem::PageAllocator>& alloc, DEFAULT_LOGGER(logger))
        : NodeStorage(nodes_class, class_storage, alloc, logger) {
    }

    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) void AddNode(std::shared_ptr<O>& node) {
    }
};

}  // namespace db
