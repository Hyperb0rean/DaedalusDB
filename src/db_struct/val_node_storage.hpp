#pragma once

#include "node.hpp"
#include "node_storage.hpp"
#include "pagelist.hpp"

namespace db {
/* TODO: Currently id generation supports reusing of old id numbers, that were previously removed,
 * need to delete this feature due to occuring conflicts with lazy relation removal, scenario:
 * Node a1, a2 were created
 * Relation R1(a1,a2) was created
 * Node a1 were deleted (due to lazy removal R1 is dangling but JOIN on R will be correct)
 * New node a1' was created and its id is a1
 * JOIN on R will produce Incorrect Behaviour
 *
 * Currently fixed but need to review it later cause I'm not sure that remade logic correct
 */

class ValNodeStorage : public NodeStorage {

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

        [[nodiscard]] auto InPageOffset() const noexcept -> mem::PageOffset {
            return inner_offset_;
        }
        [[nodiscard]] auto Page() const noexcept -> mem::PageList::PageIterator {
            return current_page_;
        }

        [[nodiscard]] auto GetNodesInPage() const -> size_t {
            return (mem::kPageSize - sizeof(mem::Page)) / Size();
        }

        [[nodiscard]] auto GetInPageIndex() const -> ts::ObjectId {
            return (inner_offset_ - sizeof(mem::Page)) / Size();
        }

    public:
        [[nodiscard]] auto Size() const -> size_t {
            return sizeof(mem::Magic) + sizeof(ts::ObjectId) + node_class_->Size().value();
        }

        [[nodiscard]] auto Id() -> ts::ObjectId {
            return file_->Read<ts::ObjectId>(GetRealOffset() +
                                             static_cast<mem::Offset>(sizeof(mem::Magic)));
        }

        [[nodiscard]] auto GetRealOffset() -> mem::Offset {
            return mem::GetOffset(current_page_->index_, inner_offset_);
        }

    private:
        auto RegenerateEnd() -> void {
            auto back = mem::ReadPage(mem::Page(page_list_.Back()), file_);
            end_offset_ = mem::GetOffset(back.index_, back.initialized_offset_);
        }

        auto State() -> ObjectState {
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

        auto Advance() -> void {
            RegenerateEnd();
            do {
                if (GetRealOffset() >= end_offset_) {
                    return;
                }
                if (GetInPageIndex() + 1 <= GetNodesInPage()) {
                    inner_offset_ += Size();
                } else {
                    ++current_page_;
                    inner_offset_ = sizeof(mem::Page);
                }
            } while (State() != ObjectState::kValid);
        }

        auto Retreat() -> void {
            RegenerateEnd();
            do {
                if (page_list_.Front() == current_page_->index_ &&
                    inner_offset_ == sizeof(mem::Page)) {
                    return;
                }
                if (GetInPageIndex() >= 1) {
                    inner_offset_ -= Size();
                } else {
                    --current_page_;
                    inner_offset_ = static_cast<mem::PageOffset>(Size() * (GetNodesInPage() - 1) +
                                                                 sizeof(mem::Page));
                }
            } while (State() != ObjectState::kValid);
        }

        auto Read() -> void {
            curr_ = util::MakePtr<Node>(magic_, node_class_, file_, GetRealOffset());
        }

    public:
        friend ValNodeStorage;
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Node;
        using difference_type = ts::ObjectId;
        using pointer = Node::Ptr;
        using reference = Node&;

        NodeIterator(mem::Magic magic, ts::Class::Ptr& node_class, mem::File::Ptr& file,
                     mem::PageList& page_list, mem::PageList::PageIterator it,
                     mem::PageOffset offset)
            : magic_(magic),
              node_class_(node_class),
              file_(file),
              page_list_(page_list),
              inner_offset_(offset),
              current_page_(it) {
            if (!page_list_.IsEmpty()) {
                RegenerateEnd();
                while (State() == ObjectState::kFree) {
                    Advance();
                }
                if (GetRealOffset() != end_offset_) {
                    Read();
                }
            } else {
                current_page_ = page_list_.End();
            }
        }
        auto operator++() -> NodeIterator& {
            Advance();
            return *this;
        }
        auto operator++(int) -> NodeIterator {
            auto temp = *this;
            Advance();
            return temp;
        }

        auto operator--() -> NodeIterator& {
            Retreat();
            return *this;
        }
        auto operator--(int) -> NodeIterator {
            auto temp = *this;
            Retreat();
            return temp;
        }

        auto operator*() -> reference {
            Read();
            return *curr_;
        }
        auto operator->() -> pointer {
            Read();
            return curr_;
        }

        auto operator==(const NodeIterator& other) const -> bool {
            if (current_page_ == page_list_.End() && current_page_ == other.current_page_) {
                return true;
            } else {
                return current_page_ == other.current_page_ && inner_offset_ == other.inner_offset_;
            }
        }
        auto operator!=(const NodeIterator& other) const -> bool {
            return !(*this == other);
        }
    };

    template <ts::ClassLike C>
    ValNodeStorage(const util::Ptr<C>& nodes_class, ClassStorage::Ptr& class_storage,
                   mem::PageAllocator::Ptr& alloc, DEFAULT_LOGGER(logger))
        : NodeStorage(nodes_class, class_storage, alloc, logger) {
        DEBUG("Val Node storage initialized with class: ", ts::ClassObject(nodes_class).ToString());
    }

    auto Begin() -> NodeIterator {
        return NodeIterator(GetHeader().magic_, nodes_class_, alloc_->GetFile(), data_page_list_,
                            data_page_list_.Begin(), sizeof(mem::Page));
    }

    auto End() -> NodeIterator {
        return NodeIterator(GetHeader().magic_, nodes_class_, alloc_->GetFile(), data_page_list_,
                            data_page_list_.IteratorTo(GetBack().index_),
                            GetBack().initialized_offset_);
    }

private:
    template <ts::ObjectLike O>
    auto WrtiteIntoFree(mem::Page& back, mem::ClassHeader& header, Node& next_free,
                        util::Ptr<O>& node) -> ts::ObjectId {
        DEBUG("Back in free ", back);
        auto id = header.ReadNodeId(alloc_->GetFile()).id_;
        DEBUG("Rewrited id: ", id);
        DEBUG("Found free space: ", next_free.NextFree());
        auto metaobject = Node(header.ReadMagic(alloc_->GetFile()).magic_, id, node);
        metaobject.Write(alloc_->GetFile(), mem::GetOffset(back.index_, back.free_offset_));
        back.free_offset_ = next_free.NextFree();
        back.actual_size_ += metaobject.Size();
        return metaobject.Id();
    }

    template <ts::ObjectLike O>
    auto WriteIntoInvalid(mem::Page& back, mem::ClassHeader& header, util::Ptr<O>& node)
        -> ts::ObjectId {
        DEBUG("Back in invalid ", back);
        auto id = header.ReadNodeId(alloc_->GetFile()).id_;
        auto metaobject = Node(header.ReadMagic(alloc_->GetFile()).magic_, id, node);
        if (back.initialized_offset_ + metaobject.Size() >= mem::kPageSize) {
            DEBUG("Allocation");
            AllocatePage();
            back = GetBack();
        }
        DEBUG("Initializing new memory on id: ", id,
              ", offset: ", mem::GetOffset(back.index_, back.initialized_offset_));

        metaobject.Write(alloc_->GetFile(), mem::GetOffset(back.index_, back.free_offset_));
        back.free_offset_ += metaobject.Size();
        back.initialized_offset_ += metaobject.Size();
        back.actual_size_ += metaobject.Size();
        return metaobject.Id();
    }

public:
    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) auto AddNode(util::Ptr<O>& node) -> void {

        if (node->Size() + sizeof(mem::Magic) + sizeof(ts::ObjectId) + sizeof(mem::Page) >=
            mem::kPageSize) {
            throw error::NotImplemented("Too big Object");
        }

        INFO("Addding node: ", node->ToString());
        auto header = GetHeader();
        auto back = GetBack();
        auto next_free = Node(header.ReadMagic(alloc_->GetFile()).magic_, nodes_class_,
                              alloc_->GetFile(), mem::GetOffset(back.index_, back.free_offset_));
        DEBUG("Free: ", next_free.ToString());

        ts::ObjectId id;
        switch (next_free.State()) {
            case ObjectState::kFree: {
                // if (back.initialized_offset_ == back.free_offset_) {
                //     id = WriteIntoInvalid(back, header, node);
                // } else {
                // BUG: Need to generate magics for class;
                id = WrtiteIntoFree(back, header, next_free, node);
                // }
            } break;
            case ObjectState::kInvalid: {
                id = WriteIntoInvalid(back, header, node);
            } break;
            case ObjectState::kValid:
                throw error::RuntimeError("Already occupied memory");
            default:
                break;
        }
        INFO("Successfully added node with id: ", id);

        mem::WritePage(back, alloc_->GetFile());
        DEBUG("Back ", back);
        header.WriteNodeId(alloc_->GetFile(), ++header.ReadNodeId(alloc_->GetFile()).id_);
        DEBUG("Id: ", GetHeader().id_);
    }

    template <typename Predicate, typename Functor>
    requires requires(Predicate pred, Functor functor, NodeIterator iter) {
        { pred(iter) } -> std::convertible_to<bool>;
        {functor(iter)};
    }
    auto VisitNodes(Predicate predicate, Functor functor) -> void {
        DEBUG("Visiting nodes..");
        auto end = End();
        for (auto node_it = Begin(); node_it != end; ++node_it) {
            if (predicate(node_it)) {
                DEBUG("Node: ", node_it->ToString());
                functor(node_it);
            }
        }
    }

    template <typename Predicate>
    requires std::is_invocable_r_v<bool, Predicate, NodeIterator>
    auto RemoveNodesIf(Predicate predicate) -> void {
        DEBUG("Removing nodes..");

        auto end = End();
        // size_t count = 0;
        std::vector<mem::PageIndex> free_pages;
        for (auto node_it = Begin(); node_it != end; ++node_it) {
            if (predicate(node_it)) {
                DEBUG("Removing node ", node_it.Id());
                auto page = mem::ReadPage(*node_it.Page(), alloc_->GetFile());
                auto node = *node_it;

                page.actual_size_ -= node.Size();
                node.Free(page.free_offset_);
                node.Write(alloc_->GetFile(),
                           mem::GetOffset(node_it.Page()->index_, node_it.InPageOffset()));
                DEBUG("Node: ", node.ToString());
                page.free_offset_ = node_it.InPageOffset();
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
        // TODO: see above
        // auto header = GetHeader();
        // header.WriteNodeCount(alloc_->GetFile(), header.nodes_ - count);
    }
};
}  // namespace db