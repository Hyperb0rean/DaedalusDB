#pragma once

#include "node_storage.hpp"

namespace db {

class VarNodeStorage : public NodeStorage {

public:
    class NodeIterator {
    private:
        mem::Magic magic_;
        std::shared_ptr<ts::Class> node_class_;
        std::shared_ptr<mem::File> file_;
        mem::PageList& page_list_;

        mem::PageOffset inner_offset_;
        mem::PageList::PageIterator current_page_;

        mem::Offset end_offset_;
        mem::Offset real_offset_;

        std::shared_ptr<Node> curr_;

    public:
        [[nodiscard]] ObjectId Id() {
            return file_->Read<ObjectId>(real_offset_ + sizeof(mem::Magic));
        }

    private:
        [[nodiscard]] mem::PageOffset InPageOffset() const noexcept {
            return inner_offset_;
        }
        [[nodiscard]] mem::PageList::PageIterator Page() const noexcept {
            return current_page_;
        }

        void RegenerateEnd() {
            auto back = mem::ReadPage(mem::Page(page_list_.Back()), file_);
            end_offset_ = mem::GetOffset(back.index_, back.initialized_offset_);
        }

        ObjectState State() {
            auto magic = file_->Read<mem::Magic>(real_offset_);
            if (magic == magic_) {
                return ObjectState::kValid;
            } else if (magic == ~magic_) {
                return ObjectState::kFree;
            } else {
                return ObjectState::kInvalid;
                // throw error::RuntimeError("Invalid iterator");
            }
        }

        void Next() {
            if (real_offset_ >= end_offset_) {
                return;
            }
            auto offset = file_->Read<mem::PageOffset>(real_offset_ + sizeof(mem::Magic));
            if (inner_offset_ + offset >= mem::kPageSize) {
                ++current_page_;
                inner_offset_ = current_page_.ReadPage().initialized_offset_;
            } else {
                inner_offset_ += offset;
                real_offset_ += offset;
            }
        }

        void Advance() {
            do {
                Next();
            } while (State() != ObjectState::kValid);
        }

        void Read() {
            curr_ = std::make_shared<Node>(magic_, node_class_, file_, real_offset_);
        }

    public:
        friend VarNodeStorage;
        using iterator_category = std::forward_iterator_tag;
        using value_type = Node;
        using difference_type = ObjectId;
        using pointer = std::shared_ptr<Node>;
        using reference = Node&;

        NodeIterator(mem::Magic magic, std::shared_ptr<ts::Class>& node_class,
                     std::shared_ptr<mem::File>& file, mem::PageList& page_list,
                     mem::PageOffset inner_offset)
            : magic_(magic),
              node_class_(node_class),
              file_(file),
              page_list_(page_list),
              inner_offset_(inner_offset),
              current_page_(page_list.Begin()) {

            if (page_list_.IsEmpty()) {
                real_offset_ = SIZE_MAX;
                current_page_ = page_list_.End();
                return;
            }

            real_offset_ = mem::GetOffset(current_page_.Index(), inner_offset_);
            RegenerateEnd();
            Read();
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

        reference operator*() {
            Read();
            return *curr_;
        }
        pointer operator->() {
            Read();
            return curr_;
        }

        bool operator==(const NodeIterator& other) const {
            return real_offset_ == other.real_offset_;
        }
        bool operator!=(const NodeIterator& other) const {
            return !(*this == other);
        }
    };

    template <ts::ClassLike C>
    VarNodeStorage(std::shared_ptr<C> nodes_class, std::shared_ptr<ClassStorage>& class_storage,
                   std::shared_ptr<mem::PageAllocator>& alloc, DEFAULT_LOGGER(logger))
        : NodeStorage(nodes_class, class_storage, alloc, logger) {
        DEBUG("Var Node storage initialized with class: ", ts::ClassObject(nodes_class).ToString());
    }

    NodeIterator Begin() {
        return NodeIterator(GetHeader().magic_, nodes_class_, alloc_->GetFile(), data_page_list_,
                            GetFront().initialized_offset_);
    }

    NodeIterator End() {
        if (data_page_list_.IsEmpty()) {
            return Begin();
        }
        return NodeIterator(GetHeader().magic_, nodes_class_, alloc_->GetFile(), data_page_list_,
                            GetBack().free_offset_);
    }

    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) void AddNode(std::shared_ptr<O>& node) {

        if (node->Size() + sizeof(mem::Magic) + sizeof(ObjectId) + sizeof(mem::Page) >=
            mem::kPageSize) {
            throw error::NotImplemented("Too big Object");
        }

        INFO("Addding node: ", node->ToString());
        auto header = GetHeader();
        auto back = GetBack();
        auto count = header.ReadNodeCount(alloc_->GetFile()).nodes_;

        // TODO

        mem::WritePage(back, alloc_->GetFile());
        DEBUG("Back ", back);
        header.WriteNodeCount(alloc_->GetFile(), ++count);
        DEBUG("Count ", GetHeader().nodes_);
    }

    template <typename Predicate, typename Functor>
    requires requires(Predicate pred, Functor functor, NodeIterator iter) {
        { pred(iter) } -> std::convertible_to<bool>;
        {functor(*iter)};
    }
    void VisitNodes(Predicate predicate, Functor functor) {
        DEBUG("Visiting nodes..");
        DEBUG("Pages count ", data_page_list_.GetPagesCount());
        if (data_page_list_.IsEmpty()) {
            DEBUG("Empty list");
            return;
        }
        DEBUG("Begin page: ", *data_page_list_.Begin());
        auto end = End();
        for (auto node_it = Begin(); node_it != end; ++node_it) {
            if (predicate(node_it)) {
                DEBUG("Node: ", node_it->ToString());
                functor(*node_it);
            }
        }
    }

    template <typename Predicate>
    requires std::is_invocable_r_v<bool, Predicate, NodeIterator>
    void RemoveNodesIf(Predicate predicate) {
        DEBUG("Removing nodes..");
        DEBUG("Pages count ", data_page_list_.GetPagesCount());
        if (data_page_list_.IsEmpty()) {
            DEBUG("Empty list");
            return;
        }
        auto end = End();
        size_t count = 0;
        std::vector<mem::PageIndex> free_pages;
        for (auto node_it = Begin(); node_it != end; ++node_it) {
            if (predicate(node_it)) {
                DEBUG("Removing node ", node_it.Id());
                DEBUG("Node: ", node_it->ToString());
                auto page = mem::ReadPage(*node_it.Page(), alloc_->GetFile());

                // TODO

                DEBUG("Page: ", page);
                mem::WritePage(page, alloc_->GetFile());
                if (page.actual_size_ == 0) {
                    INFO("Deallocated page", page);
                    free_pages.push_back(page.index_);
                }
                ++count;
            }
        }
        for (auto id : free_pages) {
            FreePage(id);
        }
        auto header = GetHeader();
        header.WriteNodeCount(alloc_->GetFile(), header.nodes_ - count);
    }
};
}  // namespace db