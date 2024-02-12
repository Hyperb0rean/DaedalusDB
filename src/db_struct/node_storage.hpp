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

    template <typename M>
    class ObjectIterator {
    private:
        std::shared_ptr<mem::File> file_;
        mem::PageList& page_list_;
        size_t node_size_;

        mem::PageOffset inner_offset_;
        mem::PageList::PageIterator current_page_;
        ObjectId id_;

        std::shared_ptr<MetaObject<M>> curr_;

        size_t GetNodesInPage() {
            return (mem::kPageSize - sizeof(mem::Page)) / node_size_;
        }

        ObjectId GetInPageIndex() {
            return (inner_offset_ - sizeof(mem::Page)) / node_size_;
        }

        void Advance() {
            if (current_page_ == page_list_.End()) {
                return;
            }

            ++id_;
            if (GetInPageIndex() + 1 < GetNodesInPage()) {
                inner_offset_ += node_size_;
            } else {
                ++current_page_;
                inner_offset_ = sizeof(mem::Page);
            }
        }

        void Retreat() {
            if (current_page_ == page_list_.End()) {
                return;
            }

            --id_;
            if (GetInPageIndex() >= 1) {
                inner_offset_ -= node_size_;
            } else {
                --current_page_;
                inner_offset_ = node_size_ * (GetNodesInPage() - 1) + sizeof(mem::Page);
            }
        }

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = MetaObject<M>;
        using difference_type = size_t;
        using pointer = std::shared_ptr<MetaObject<M>>;
        using reference = MetaObject<M>&;

        ObjectIterator() {
        }

        ObjectIterator(std::shared_ptr<mem::File>& file, mem::PageList& page_list, size_t node_size,
                       ObjectId id)
            : file_(file),
              page_list_(page_list),
              node_size_(node_size),
              inner_offset_(sizeof(mem::Page)),
              current_page_(page_list_.Begin()),
              id_(0) {

            for (auto& page_it : page_list_) {
                if (id_ + GetNodesInPage() <= id) {
                    current_page_ = page_it;
                    id_ += GetNodesInPage();
                } else {
                    break;
                }
            }

            while (id_ < id) {
                Advance();
            }
        }
        ObjectIterator& operator++() {
            Advance();
            return *this;
        }
        ObjectIterator operator++(int) {
            auto temp = *this;
            Advance();
            return temp;
        }

        ObjectIterator& operator--() {
            Retreat();
            return *this;
        }
        ObjectIterator operator--(int) {
            auto temp = *this;
            Retreat();
            return temp;
        }

        reference operator*() {
            curr_->Read(file_, mem::GetOffset(current_page_->index_, inner_offset_));
            return *curr_;
        }
        pointer operator->() {
            curr_->Read(file_, mem::GetOffset(current_page_->index_, inner_offset_));
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
