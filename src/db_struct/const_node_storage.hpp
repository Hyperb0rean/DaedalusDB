#pragma once

#include "node_storage.hpp"

namespace db {

class ConstantSizeNodeStorage : public NodeStorage {

    mem::PageOffset GetPageOffsetById(ObjectId id) {
        return sizeof(mem::Page) + id * nodes_class_->Size().value();
    }

    template <ts::ObjectLike O>
    void WriteIntoFree(mem::Page& back, mem::ClassHeader& header, std::shared_ptr<O>& node,
                       MetaObject& jumper) {

        DEBUG("Found free space: ", jumper.Id());
        auto metaobject = MetaObject(header.ReadMagic(alloc_->GetFile()).magic_, jumper.Id(), node);
        metaobject.Write(alloc_->GetFile(), mem::GetOffset(back.index_, back.free_offset_));

        back.free_offset_ = GetPageOffsetById(jumper.Id());
        mem::WritePage(back, alloc_->GetFile());

        header.WriteNodeCount(alloc_->GetFile(),
                              ++(header.ReadNodeCount(alloc_->GetFile()).nodes_));
        DEBUG("Written offsets free: ", back.free_offset_);
        INFO("Successfully added node with id: ", metaobject.Id());
    }

    template <ts::ObjectLike O>
    void WriteIntoInvalid(mem::Page& back, mem::ClassHeader& header, std::shared_ptr<O>& node) {
        auto count = header.ReadNodeCount(alloc_->GetFile()).nodes_;
        auto metaobject = MetaObject(header.ReadMagic(alloc_->GetFile()).magic_, count, node);

        if (back.initialized_offset_ + metaobject.Size() > mem::kPageSize) {
            DEBUG("Allocation");
            AllocatePage();
            back = GetBack();
        }
        DEBUG("Initializing new memory on id: ", count,
              ", offset: ", mem::GetOffset(back.index_, back.initialized_offset_));

        metaobject.Write(alloc_->GetFile(), mem::GetOffset(back.index_, back.free_offset_));

        back.free_offset_ += metaobject.Size();
        back.initialized_offset_ += metaobject.Size();
        mem::WritePage(back, alloc_->GetFile());
        DEBUG("Written offsets free: ", back.free_offset_, ", init: ", back.initialized_offset_);

        header.WriteNodeCount(alloc_->GetFile(), ++count);
        INFO("Successfully added node with id: ", metaobject.Id());
    }

public:
    class ObjectIterator {
    private:
        std::shared_ptr<mem::File> file_;
        mem::PageList& page_list_;
        size_t node_size_;

        mem::PageOffset inner_offset_;
        mem::PageList::PageIterator current_page_;
        ObjectId id_;

        std::shared_ptr<MetaObject> curr_;

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
        using value_type = MetaObject;
        using difference_type = size_t;
        using pointer = std::shared_ptr<MetaObject>;
        using reference = MetaObject&;

        ObjectIterator(std::shared_ptr<mem::File>& file, mem::PageList& page_list, size_t node_size,
                       ObjectId id)
            : file_(file),
              page_list_(page_list),
              node_size_(node_size),
              inner_offset_(sizeof(mem::Page)),
              current_page_(page_list_.Begin()),
              id_(0) {

            for (auto page_it = page_list_.Begin(); page_it != page_list.End(); ++page_it) {
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

    template <ts::ClassLike C>
    ConstantSizeNodeStorage(std::shared_ptr<C> nodes_class,
                            std::shared_ptr<ClassStorage>& class_storage,
                            std::shared_ptr<mem::PageAllocator>& alloc, DEFAULT_LOGGER(logger))
        : NodeStorage(nodes_class, class_storage, alloc, logger) {
        DEBUG("Constant Node storage initialized with class: ",
              ts::ClassObject(nodes_class).ToString());
    }

    ObjectIterator Begin() {
        return ObjectIterator(alloc_->GetFile(), data_page_list_, nodes_class_->Size().value(), 0);
    }

    ObjectIterator End() {
        return ObjectIterator(alloc_->GetFile(), data_page_list_, nodes_class_->Size().value(),
                              GetHeader().ReadNodeCount(alloc_->GetFile()).nodes_);
    }

    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) void AddNode(std::shared_ptr<O>& node) {

        if (node->Size() + sizeof(mem::Magic) + sizeof(ObjectId) + sizeof(mem::Page) >
            mem::kPageSize) {
            throw error::NotImplemented("Too big Object");
        }
        INFO("Addding node: ", node->ToString());
        auto header = GetHeader();
        auto back = GetBack();
        auto jumper = MetaObject(header.ReadMagic(alloc_->GetFile()).magic_, nodes_class_,
                                 alloc_->GetFile(), mem::GetOffset(back.index_, back.free_offset_));
        DEBUG("Jumper: ", jumper.ToString());
        switch (jumper.State()) {
            case ObjectState::kFree: {
                WriteIntoFree(back, header, node, jumper);
                return;
            }
            case ObjectState::kInvalid: {
                WriteIntoInvalid(back, header, node);
                return;
            }
            case ObjectState::kValid:
                throw error::RuntimeError("Already occupied memory");
            default:
                return;
        }
    }
};
}  // namespace db