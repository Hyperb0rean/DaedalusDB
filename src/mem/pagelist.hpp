#pragma once

#include <iostream>

#include "allocator.hpp"

namespace mem {

class PageList {

    std::shared_ptr<PageAllocator> alloc_;
    Offset dummy_offset_;
    size_t pages_count_;
    std::shared_ptr<util::Logger> logger_;

    void DecrementCount() {
        alloc_->GetFile()->Write<size_t>(--pages_count_, GetCountFromSentinel(dummy_offset_));
        logger_->Debug("Decremented page count, current: " + std::to_string(pages_count_));
    }
    void IncrementCount() {
        alloc_->GetFile()->Write<size_t>(++pages_count_, GetCountFromSentinel(dummy_offset_));
        logger_->Debug("Incremented page count, current: " + std::to_string(pages_count_));
    }

public:
    [[nodiscard]] size_t GetPagesCount() const {
        logger_->Debug("Pages in list: " + std::to_string(pages_count_));
        return pages_count_;
    }

    class PageIterator {
        std::shared_ptr<PageAllocator> alloc_;
        Offset dummy_offset_;
        Page curr_;

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
            curr_ = ReadPage(curr_.previous_page_index_);
            return *this;
        }
        PageIterator operator++(int) {
            auto temp = *this;
            curr_ = ReadPage(curr_.previous_page_index_);
            return temp;
        }

        PageIterator& operator--() {
            curr_ = ReadPage(curr_.next_page_index_);
            return *this;
        }
        PageIterator operator--(int) {
            auto temp = *this;
            curr_ = ReadPage(curr_.next_page_index_);
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
            if (index < kDummyIndex) {
                return Page(index).ReadPage(alloc_->GetFile(), alloc_->GetCr3());
            } else {
                return alloc_->GetFile()->Read<Page>(dummy_offset_);
            }
        }

        void WritePage() {
            if (curr_.index_ < kDummyIndex) {
                curr_.WritePage(alloc_->GetFile(), alloc_->GetCr3());
            } else {
                alloc_->GetFile()->Write<Page>(curr_, dummy_offset_);
            }
        }
    };

    PageList() {
    }

    PageList(const std::shared_ptr<PageAllocator>& alloc, Offset dummy_offset,
             std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : alloc_(alloc), dummy_offset_(dummy_offset), logger_(logger) {
        pages_count_ = alloc_->GetFile()->Read<size_t>(GetCountFromSentinel(dummy_offset));
    }

    void Unlink(PageIndex index) {
        auto it = PageIterator(alloc_, index, dummy_offset_);
        if (it->next_page_index_ == it->index_ && it->previous_page_index_ == it->index_) {
            return;
        }

        logger_->Debug("Unlinking page " + std::to_string(index));

        auto prev = PageIterator(alloc_, it->previous_page_index_, dummy_offset_);
        auto next = PageIterator(alloc_, it->next_page_index_, dummy_offset_);
        prev->next_page_index_ = next->index_;
        next->previous_page_index_ = prev->index_;
        it->previous_page_index_ = it->index_;
        it->next_page_index_ = it->index_;

        if (GetPagesCount() == 1) {
            next->next_page_index_ = next->index_;
            next->previous_page_index_ = next->index_;
        }

        it.WritePage();
        prev.WritePage();
        next.WritePage();
        DecrementCount();
    }

    void LinkBefore(PageIndex other_index, PageIndex index) {
        // other_index must be from list, index must not be from list
        logger_->Debug("Linking page " + std::to_string(index) + " before " +
                       std::to_string(other_index));

        auto it = PageIterator(alloc_, index, dummy_offset_);
        auto other = PageIterator(alloc_, other_index, dummy_offset_);
        auto prev = PageIterator(alloc_, other->previous_page_index_, dummy_offset_);

        it->next_page_index_ = other->index_;
        it->previous_page_index_ = prev->index_;
        prev->next_page_index_ = it->index_;
        other->previous_page_index_ = it->index_;

        if (GetPagesCount() == 0) {
            other->next_page_index_ = it->index_;
        }

        it.WritePage();
        prev.WritePage();
        other.WritePage();

        IncrementCount();
    }

    // Index must be in the list
    PageIterator IteratorTo(PageIndex index) {
        return PageIterator(alloc_, index, dummy_offset_);
    }

    void PushBack(PageIndex index) {
        logger_->Debug("Push back");
        LinkBefore(IteratorTo(mem::kDummyIndex)->next_page_index_, index);
    }

    void PushFront(PageIndex index) {
        logger_->Debug("Push front");
        LinkBefore(mem::kDummyIndex, index);
    }

    void PopBack() {
        logger_->Debug("Pop back");
        Unlink(IteratorTo(mem::kDummyIndex)->next_page_index_);
    }
    void PopFront() {
        logger_->Debug("Pop front");
        Unlink(IteratorTo(mem::kDummyIndex)->previous_page_index_);
    }

    bool IsEmpty() const {
        return GetPagesCount() == 0;
    }

    PageIterator Begin() {
        return PageIterator(alloc_,
                            alloc_->GetFile()->Read<Page>(dummy_offset_).previous_page_index_,
                            dummy_offset_);
    }
    PageIterator End() {
        return PageIterator(alloc_, alloc_->GetFile()->Read<Page>(dummy_offset_).index_,
                            dummy_offset_);
    }

    PageIndex Front() {
        return alloc_->GetFile()->Read<Page>(dummy_offset_).previous_page_index_;
    }

    PageIndex Back() {
        return alloc_->GetFile()->Read<Page>(dummy_offset_).next_page_index_;
    }
};

PageList::PageIterator begin(PageList& list) {
    return list.Begin();
}

PageList::PageIterator end(PageList& list) {
    return list.End();
}

}  // namespace mem