#pragma once

#include "node.hpp"
#include "node_storage.hpp"

namespace db {

class VarNodeStorage : public NodeStorage {

public:
    class NodeIterator {
    private:
        mem::Magic magic_;
        ts::Class::Ptr node_class_;
        mem::File::Ptr file_;
        mem::PageList& page_list_;

        mem::PageOffset inner_offset_;
        mem::PageList::PageIterator current_page_;

        mem::Offset end_offset_;

        Node::Ptr curr_;

    public:
        [[nodiscard]] ObjectId Id() {
            return file_->Read<ObjectId>(GetRealOffset() +
                                         static_cast<mem::Offset>(sizeof(mem::Magic)));
        }
        [[nodiscard]] mem::Offset GetRealOffset() {
            return mem::GetOffset(current_page_->index_, inner_offset_);
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
            end_offset_ = mem::GetOffset(back.index_, back.free_offset_);
        }

        ObjectState State() {
            auto magic = file_->Read<mem::Magic>(GetRealOffset());
            if (magic == magic_) {
                return ObjectState::kValid;
            } else if (magic == ~magic_) {
                return ObjectState::kFree;
            } else {
                return ObjectState::kInvalid;
                // throw error::RuntimeError("Invalid iterator");
            }
        }

        void Read() {
            curr_ = util::MakePtr<Node>(magic_, node_class_, file_, GetRealOffset());
        }

        void Advance() {
            if (State() == ObjectState::kValid) {
                inner_offset_ += curr_->Size();
            }
            while (State() != ObjectState::kValid) {
                if (GetRealOffset() >= end_offset_) {
                    return;
                }
                auto offset = file_->Read<mem::PageOffset>(
                    GetRealOffset() + static_cast<mem::PageOffset>(sizeof(mem::Magic)));
                // offset == 0
                if (State() == ObjectState::kInvalid) {
                    ++current_page_;
                    inner_offset_ = current_page_->initialized_offset_;
                } else {
                    inner_offset_ = offset;
                }
            }
            if (State() == ObjectState::kValid) {
                Read();
            }
        }

    public:
        friend VarNodeStorage;
        using iterator_category = std::forward_iterator_tag;
        using value_type = Node;
        using difference_type = ObjectId;
        using pointer = Node::Ptr;
        using reference = Node&;

        NodeIterator(mem::Magic magic, ts::Class::Ptr& node_class, mem::File::Ptr& file,
                     mem::PageList& page_list, mem::PageIndex index, mem::PageOffset inner_offset)
            : magic_(magic),
              node_class_(node_class),
              file_(file),
              page_list_(page_list),
              inner_offset_(inner_offset),
              current_page_(page_list.IteratorTo(index)) {

            if (!page_list_.IsEmpty()) {
                RegenerateEnd();
                Read();
                while (State() == ObjectState::kFree) {
                    Advance();
                }

            } else {
                current_page_ = page_list_.End();
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

        reference operator*() {
            return *curr_;
        }
        pointer operator->() {
            return curr_;
        }

        bool operator==(const NodeIterator& other) const {
            if (current_page_ == page_list_.End() && current_page_ == other.current_page_) {
                return true;
            } else {
                return current_page_ == other.current_page_ && inner_offset_ == other.inner_offset_;
            }
        }
        bool operator!=(const NodeIterator& other) const {
            return !(*this == other);
        }
    };

    template <ts::ClassLike C>
    VarNodeStorage(util::Ptr<C> nodes_class, util::Ptr<ClassStorage>& class_storage,
                   mem::PageAllocator::Ptr& alloc, DEFAULT_LOGGER(logger))
        : NodeStorage(nodes_class, class_storage, alloc, logger) {
        DEBUG("Var Node storage initialized with class: ", ts::ClassObject(nodes_class).ToString());
    }

    NodeIterator Begin() {
        return NodeIterator(GetHeader().magic_, nodes_class_, alloc_->GetFile(), data_page_list_,
                            GetFront().index_, GetFront().initialized_offset_);
    }

    NodeIterator End() {
        return NodeIterator(GetHeader().magic_, nodes_class_, alloc_->GetFile(), data_page_list_,
                            GetBack().index_, GetBack().free_offset_);
    }

private:
    mem::Offset GetNewNodeOffset(size_t node_size) {
        auto back = GetBack();
        if (back.free_offset_ + node_size >= mem::kPageSize) {
            DEBUG("Allocation");
            // Maybe should think about the case | valid | new | -> | free | new |

            AllocatePage();
            back = GetBack();
        }
        return mem::GetOffset(back.index_, back.free_offset_);
    }

public:
    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) void AddNode(util::Ptr<O>& node) {

        if (node->Size() + sizeof(mem::Magic) + sizeof(ObjectId) + sizeof(mem::Page) >=
            mem::kPageSize) {
            throw error::NotImplemented("Too big Object");
        }

        INFO("Addding node: ", node->ToString());
        auto count = GetHeader().ReadNodeCount(alloc_->GetFile()).nodes_;
        // TODO: fix id's uniqueness
        auto metaobject = Node(GetHeader().ReadMagic(alloc_->GetFile()).magic_, count, node);
        auto node_offset = GetNewNodeOffset(metaobject.Size());
        auto back = GetBack();
        DEBUG("Initializing new memory on id: ", count, ", offset: ", node_offset);

        metaobject.Write(alloc_->GetFile(), node_offset);
        back.free_offset_ += metaobject.Size();
        back.actual_size_ += metaobject.Size();

        INFO("Successfully added node with id: ", count);

        mem::WritePage(back, alloc_->GetFile());
        DEBUG("Back ", back);
        GetHeader().WriteNodeCount(alloc_->GetFile(), ++count);
        DEBUG("Count ", GetHeader().nodes_);
    }

    template <typename Predicate, typename Functor>
    requires requires(Predicate pred, Functor functor, NodeIterator iter) {
        { pred(iter) } -> std::convertible_to<bool>;
        {functor(*iter)};
    }
    void VisitNodes(Predicate predicate, Functor functor) {
        DEBUG("Visiting nodes..");
        DEBUG("Begin page: ", *data_page_list_.Begin());
        DEBUG("End page: ", GetBack());

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
        auto end = End();

        // Actually count for vals means id but not actually node storage size, probably should
        // change for separate fields
        // size_t count = 0;
        std::vector<mem::PageIndex> free_pages;
        for (auto node_it = Begin(); node_it != end;) {
            auto current_it = node_it++;
            if (predicate(current_it)) {
                DEBUG("Node: ", current_it->ToString());
                auto page = mem::ReadPage(*current_it.Page(), alloc_->GetFile());
                auto node = *current_it;

                page.actual_size_ -= node.Size();

                // Not working as intended this should be a pointer from which we will iterate in
                // page for some optimizations
                page.initialized_offset_ =
                    std::min(current_it.InPageOffset(), page.initialized_offset_);
                node.Free(current_it.InPageOffset() + static_cast<mem::PageOffset>(node.Size()));
                node.Write(alloc_->GetFile(),
                           mem::GetOffset(current_it.Page()->index_, current_it.InPageOffset()));

                DEBUG("Page: ", page);
                mem::WritePage(page, alloc_->GetFile());
                if (page.actual_size_ == 0) {
                    INFO("Deallocated page", page);
                    free_pages.push_back(page.index_);
                }
                // ++count;
            }
        }
        for (auto id : free_pages) {
            FreePage(id);
        }
        // auto header = GetHeader();
        // header.WriteNodeCount(alloc_->GetFile(), header.nodes_ - count);
    }
};
}  // namespace db