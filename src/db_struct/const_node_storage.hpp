#pragma once

#include "node_storage.hpp"

namespace db {

class ConstantSizeNodeStorage : public NodeStorage {

    mem::PageOffset GetPageOffsetById(ObjectId id) {
        return sizeof(mem::Page) + id * nodes_class_->Size().value();
    }

    template <ts::ObjectLike O>
    void WriteIntoFree(mem::Page& back, mem::ClassHeader& header, std::shared_ptr<O>& node,
                       Node& jumper) {

        DEBUG("Found free space: ", jumper.Id());
        auto metaobject = Node(header.ReadMagic(alloc_->GetFile()).magic_, jumper.Id(), node);
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
        auto metaobject = Node(header.ReadMagic(alloc_->GetFile()).magic_, count, node);

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
    class NodeIterator {
    private:
        mem::Magic magic_;
        std::shared_ptr<ts::Class> node_class_;
        std::shared_ptr<mem::File> file_;
        mem::PageList& page_list_;

        mem::PageOffset inner_offset_;
        mem::PageList::PageIterator current_page_;
        ObjectId id_;

        std::shared_ptr<Node> curr_;

        size_t Size() {
            return sizeof(mem::Magic) + sizeof(ObjectId) + node_class_->Size().value();
        }

        size_t GetNodesInPage() {
            return (mem::kPageSize - sizeof(mem::Page)) / Size();
        }

        ObjectId GetInPageIndex() {
            return (inner_offset_ - sizeof(mem::Page)) / Size();
        }

        void Advance() {
            if (current_page_ == page_list_.End()) {
                return;
            }

            ++id_;
            if (GetInPageIndex() + 1 < GetNodesInPage()) {
                inner_offset_ += Size();
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
                inner_offset_ -= Size();
            } else {
                --current_page_;
                inner_offset_ = Size() * (GetNodesInPage() - 1) + sizeof(mem::Page);
            }
        }

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Node;
        using difference_type = size_t;
        using pointer = std::shared_ptr<Node>;
        using reference = Node&;

        NodeIterator(mem::Magic magic, std::shared_ptr<ts::Class>& node_class,
                     std::shared_ptr<mem::File>& file, mem::PageList& page_list, ObjectId id)
            : magic_(magic),
              node_class_(node_class),
              file_(file),
              page_list_(page_list),
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
        NodeIterator& operator++() {
            Advance();
            return *this;
        }
        NodeIterator operator++(int) {
            auto temp = *this;
            Advance();
            return temp;
        }

        NodeIterator& operator--() {
            Retreat();
            return *this;
        }
        NodeIterator operator--(int) {
            auto temp = *this;
            Retreat();
            return temp;
        }

        reference operator*() {
            curr_ = std::make_shared<Node>(magic_, node_class_, file_,
                                           mem::GetOffset(current_page_->index_, inner_offset_));
            return *curr_;
        }
        pointer operator->() {
            curr_ = std::make_shared<Node>(magic_, node_class_, file_,
                                           mem::GetOffset(current_page_->index_, inner_offset_));
            return curr_;
        }

        bool operator==(const NodeIterator& other) const {
            return id_ == other.id_;
        }
        bool operator!=(const NodeIterator& other) const {
            return !(*this == other);
        }

        ObjectId Id() const noexcept {
            return id_;
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

    NodeIterator Begin() {
        return NodeIterator(GetHeader().magic_, nodes_class_, alloc_->GetFile(), data_page_list_,
                            0);
    }

    NodeIterator End() {
        return NodeIterator(GetHeader().magic_, nodes_class_, alloc_->GetFile(), data_page_list_,
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
        auto jumper = Node(header.ReadMagic(alloc_->GetFile()).magic_, nodes_class_,
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

    template <typename Predicate, typename Functor>
    requires requires {
        requires std::is_invocable_r_v<bool, Predicate, NodeIterator>;
        requires std::is_invocable_v<Functor, Node>;
    }
    void VisitNodes(Predicate predicate, Functor functor) {
        DEBUG("Visiting nodes..");
        for (auto node_it = Begin(); node_it != End(); ++node_it) {
            if (predicate(node_it)) {
                functor(*node_it);
            }
        }
    }
};
}  // namespace db