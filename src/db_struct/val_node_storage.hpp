#pragma once

#include "../mem/pagelist.hpp"
#include "../type_system/object.hpp"
#include "node.hpp"
#include "node_storage.hpp"

namespace db {

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
        ObjectId id_;

        mem::Offset end_offset_;

        Node::Ptr curr_;

        [[nodiscard]] mem::PageOffset InPageOffset() const noexcept {
            return inner_offset_;
        }
        [[nodiscard]] mem::PageList::PageIterator Page() const noexcept {
            return current_page_;
        }

        [[nodiscard]] size_t GetNodesInPage() const {
            return (mem::kPageSize - sizeof(mem::Page)) / Size();
        }

        [[nodiscard]] ObjectId GetInPageIndex() const {
            return (inner_offset_ - sizeof(mem::Page)) / Size();
        }

    public:
        [[nodiscard]] size_t Size() const {
            return sizeof(mem::Magic) + sizeof(ObjectId) + node_class_->Size().value();
        }

        [[nodiscard]] ObjectId Id() const noexcept {
            return id_;
        }
        [[nodiscard]] mem::Offset GetRealOffset() {
            return mem::GetOffset(current_page_->index_, inner_offset_);
        }

    private:
        NodeIterator& RegenerateId() {
            switch (curr_->State()) {
                case ObjectState::kFree: {
                    id_ = (page_list_.GetPagesCount() - 1) * GetNodesInPage() + GetInPageIndex();
                    return *this;
                }
                case ObjectState::kValid: {
                    id_ = curr_->Id();
                    return *this;
                }
                case ObjectState::kInvalid:
                default:
                    throw error::BadArgument("No id");
            }
        }

        void RegenerateEnd() {
            auto back = mem::ReadPage(mem::Page(page_list_.Back()), file_);
            end_offset_ = mem::GetOffset(back.index_, back.initialized_offset_);
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

        void Advance() {
            do {
                if (GetRealOffset() >= end_offset_) {
                    return;
                }
                ++id_;
                if (GetInPageIndex() + 1 <= GetNodesInPage()) {
                    inner_offset_ += Size();
                } else {
                    ++current_page_;
                    inner_offset_ = sizeof(mem::Page);
                }
            } while (State() != ObjectState::kValid);
        }

        void Retreat() {
            do {
                if (id_ == 0) {
                    return;
                }
                --id_;
                if (GetInPageIndex() >= 1) {
                    inner_offset_ -= Size();
                } else {
                    --current_page_;
                    inner_offset_ = static_cast<mem::PageOffset>(Size() * (GetNodesInPage() - 1) +
                                                                 sizeof(mem::Page));
                }
            } while (State() != ObjectState::kValid);
        }

        void Read() {
            curr_ = util::MakePtr<Node>(magic_, node_class_, file_, GetRealOffset());
        }

    public:
        friend ValNodeStorage;
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Node;
        using difference_type = ObjectId;
        using pointer = Node::Ptr;
        using reference = Node&;

        NodeIterator(mem::Magic magic, ts::Class::Ptr& node_class, mem::File::Ptr& file,
                     mem::PageList& page_list, ObjectId id)
            : magic_(magic),
              node_class_(node_class),
              file_(file),
              page_list_(page_list),
              inner_offset_(sizeof(mem::Page)),
              current_page_(page_list.Begin()),
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
                inner_offset_ += Size();
                ++id_;
            }
            if (!page_list_.IsEmpty()) {
                RegenerateEnd();
                Read();
            } else {
                current_page_ = page_list_.End();
            }
        }

        NodeIterator(mem::Magic magic, ts::Class::Ptr& node_class, mem::File::Ptr& file,
                     mem::PageList& page_list, mem::PageList::PageIterator it,
                     mem::PageOffset offset)
            : magic_(magic),
              node_class_(node_class),
              file_(file),
              page_list_(page_list),
              inner_offset_(offset),
              current_page_(it),
              id_(0) {
            if (!page_list_.IsEmpty()) {
                RegenerateEnd();
                if (GetRealOffset() != end_offset_) {
                    Read();
                    RegenerateId();
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
            Read();
            return *curr_;
        }
        pointer operator->() {
            Read();
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
    ValNodeStorage(util::Ptr<C> nodes_class, ClassStorage::Ptr& class_storage,
                   mem::PageAllocator::Ptr& alloc, DEFAULT_LOGGER(logger))
        : NodeStorage(nodes_class, class_storage, alloc, logger) {
        DEBUG("Val Node storage initialized with class: ", ts::ClassObject(nodes_class).ToString());
    }

    NodeIterator Begin() {
        return NodeIterator(GetHeader().magic_, nodes_class_, alloc_->GetFile(), data_page_list_,
                            0);
    }

    NodeIterator End() {
        return NodeIterator(GetHeader().magic_, nodes_class_, alloc_->GetFile(), data_page_list_,
                            data_page_list_.IteratorTo(GetBack().index_),
                            GetBack().initialized_offset_);
    }

private:
    template <ts::ObjectLike O>
    ObjectId WrtiteIntoFree(mem::Page& back, mem::ClassHeader& header, Node& next_free,
                            util::Ptr<O>& node) {
        auto id = NodeIterator(header.ReadMagic(alloc_->GetFile()).magic_, nodes_class_,
                               alloc_->GetFile(), data_page_list_,
                               data_page_list_.IteratorTo(back.index_), back.free_offset_)
                      .Id();
        DEBUG("Rewrited id: ", id);
        DEBUG("Found free space: ", next_free.NextFree());
        auto metaobject = Node(header.ReadMagic(alloc_->GetFile()).magic_, id, node);
        metaobject.Write(alloc_->GetFile(), mem::GetOffset(back.index_, back.free_offset_));
        back.free_offset_ = next_free.NextFree();
        back.actual_size_ += metaobject.Size();
        return metaobject.Id();
    }

    template <ts::ObjectLike O>
    ObjectId WriteIntoInvalid(mem::Page& back, mem::ClassHeader& header, util::Ptr<O>& node) {
        auto count = header.ReadNodeCount(alloc_->GetFile()).nodes_;
        auto metaobject = Node(header.ReadMagic(alloc_->GetFile()).magic_, count, node);
        if (back.initialized_offset_ + metaobject.Size() >= mem::kPageSize) {
            DEBUG("Allocation");
            AllocatePage();
            back = GetBack();
        }
        DEBUG("Initializing new memory on id: ", count,
              ", offset: ", mem::GetOffset(back.index_, back.initialized_offset_));

        metaobject.Write(alloc_->GetFile(), mem::GetOffset(back.index_, back.free_offset_));
        back.free_offset_ += metaobject.Size();
        back.initialized_offset_ += metaobject.Size();
        back.actual_size_ += metaobject.Size();
        return metaobject.Id();
    }

public:
    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) void AddNode(util::Ptr<O>& node) {

        if (node->Size() + sizeof(mem::Magic) + sizeof(ObjectId) + sizeof(mem::Page) >=
            mem::kPageSize) {
            throw error::NotImplemented("Too big Object");
        }

        INFO("Addding node: ", node->ToString());
        auto header = GetHeader();
        auto back = GetBack();
        auto next_free = Node(header.ReadMagic(alloc_->GetFile()).magic_, nodes_class_,
                              alloc_->GetFile(), mem::GetOffset(back.index_, back.free_offset_));
        DEBUG("Free: ", next_free.ToString());

        ObjectId id;
        switch (next_free.State()) {
            case ObjectState::kFree: {
                id = WrtiteIntoFree(back, header, next_free, node);
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
        header.WriteNodeCount(alloc_->GetFile(), ++header.ReadNodeCount(alloc_->GetFile()).nodes_);
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
        size_t count = 0;
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