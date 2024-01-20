#pragma once

#include "allocator.hpp"

namespace mem {

class PageList {
    std::shared_ptr<PageAllocator> alloc_;
    Offset dummy_offset_;
    size_t pages_count_;

    void DecrementCount() {
        alloc_->GetFile()->Write<size_t>(--pages_count_, GetCountFromSentinel(dummy_offset_));
    }
    void IncrementCount() {
        alloc_->GetFile()->Write<size_t>(++pages_count_, GetCountFromSentinel(dummy_offset_));
    }

public:
    [[nodiscard]] size_t GetPagesCount() const {
        return pages_count_;
    }

    class PageIterator {
        std::shared_ptr<PageAllocator> alloc_;
        Offset dummy_offset_;
        Page curr_;

        [[nodiscard]] size_t GetMaxIndex() const {
            return alloc_->GetFile()->Read<size_t>(GetSentinelIndex(dummy_offset_));
        }

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Page;
        using difference_type = size_t;
        using pointer = Page*;
        using reference = Page&;

        PageIterator(const std::shared_ptr<PageAllocator>& alloc, PageIndex index,
                     Offset dummy_offset)
            : alloc_(alloc), dummy_offset_(dummy_offset) {
            curr_ = ReadPage(index);
        }
        PageIterator& operator++() {
            std::cerr << curr_.index_ << " -> ";
            curr_ = ReadPage(curr_.next_page_index_);
            std::cerr << curr_.index_ << "\n";
            return *this;
        }
        PageIterator operator++(int) {
            auto temp = *this;
            curr_ = ReadPage(curr_.next_page_index_);
            return temp;
        }

        PageIterator& operator--() {
            curr_ = ReadPage(curr_.previous_page_index_);
            return *this;
        }
        PageIterator operator--(int) {
            auto temp = *this;
            curr_ = ReadPage(curr_.previous_page_index_);
            return temp;
        }

        reference operator*() {
            return curr_;
        }
        pointer operator->() {
            return &curr_;
        }

        bool operator==(const PageIterator& other) const {
            return curr_.index_ == other.curr_.index_;
        }
        bool operator!=(const PageIterator& other) const {
            return !(*this == other);
        }

        [[nodiscard]] Page ReadPage(PageIndex index) {
            if (index < GetMaxIndex()) {
                return Page(index).ReadPage(alloc_->GetFile(), alloc_->GetCr3());
            } else {
                return alloc_->GetFile()->Read<Page>(dummy_offset_);
            }
        }

        void WritePage() {
            if (curr_.index_ < GetMaxIndex()) {
                curr_.WritePage(alloc_->GetFile(), alloc_->GetCr3());
            } else {
                alloc_->GetFile()->Write<Page>(curr_, dummy_offset_);
            }
        }
    };

    PageList(const std::shared_ptr<PageAllocator>& alloc, Offset dummy_offset)
        : alloc_(alloc), dummy_offset_(dummy_offset) {
        pages_count_ = alloc_->GetFile()->Read<size_t>(GetCountFromSentinel(dummy_offset));
    }

    void Unlink(PageIndex index) {
        auto it = PageIterator(alloc_, index, dummy_offset_);
        if (it->next_page_index_ == it->previous_page_index_) {
            return;
        }

        auto prev = PageIterator(alloc_, it->previous_page_index_, dummy_offset_);
        auto next = PageIterator(alloc_, it->next_page_index_, dummy_offset_);
        prev->next_page_index_ = next->index_;
        next->previous_page_index_ = prev->index_;
        it->previous_page_index_ = it->index_;
        it->next_page_index_ = it->index_;

        it.WritePage();
        prev.WritePage();
        next.WritePage();
        DecrementCount();
    }

    void LinkBefore(PageIndex other_index, PageIndex index) {
        // other_index must be from list, index must not be from list

        auto it = PageIterator(alloc_, index, dummy_offset_);
        auto other = PageIterator(alloc_, other_index, dummy_offset_);
        auto prev = PageIterator(alloc_, other->previous_page_index_, dummy_offset_);
        it->next_page_index_ = other->index_;
        it->previous_page_index_ = prev->index_;
        prev->next_page_index_ = it->index_;
        other->previous_page_index_ = it->index_;

        it.WritePage();
        other.WritePage();
        prev.WritePage();
        IncrementCount();
    }

    void PushBack(PageIndex index) {
        LinkBefore(alloc_->GetFile()->Read<Page>(dummy_offset_).next_page_index_, index);
    }

    void PushFront(PageIndex index) {
        LinkBefore(alloc_->GetFile()->Read<Page>(dummy_offset_).index_, index);
    }

    void PopBack() {
        Unlink(alloc_->GetFile()->Read<Page>(dummy_offset_).next_page_index_);
    }
    void PopFront() {
        Unlink(alloc_->GetFile()->Read<Page>(dummy_offset_).previous_page_index_);
    }

    bool IsEmpty() const {
        return GetPagesCount() == 0;
    }

    // Index must be in the list
    PageIterator IteratorTo(PageIndex index) {
        return PageIterator(alloc_, index, dummy_offset_);
    }

    PageIterator Begin() {
        return PageIterator(alloc_, alloc_->GetFile()->Read<Page>(dummy_offset_).next_page_index_,
                            dummy_offset_);
    }
    PageIterator End() {
        return PageIterator(alloc_, alloc_->GetFile()->Read<Page>(dummy_offset_).index_,
                            dummy_offset_);
    }
};

PageList::PageIterator begin(PageList& list) {
    return list.Begin();
}

PageList::PageIterator end(PageList& list) {
    return list.End();
}

}  // namespace mem