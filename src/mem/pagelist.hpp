#pragma once

#include "allocator.hpp"

namespace mem {

class PageList {
    std::shared_ptr<PageAllocator> alloc_;
    Offset dummy_offset_;

public:
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

        void Write() {
            curr_.WritePage(alloc_->GetFile(), alloc_->GetCr3());
        }

        PageIterator(const std::shared_ptr<PageAllocator>& alloc, size_t index, Offset dummy_offset)
            : alloc_(alloc), dummy_offset_(dummy_offset) {
            curr_ = ReadPage(index);
        }
        PageIterator& operator++() {
            curr_ = ReadPage(curr_.next_page_index_);
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

        [[nodiscard]] Page ReadPage(size_t index) {
            if (index < alloc_->GetPagesCount() && index > 0) {
                return Page(index).ReadPage(alloc_->GetFile(), alloc_->GetCr3());
            } else {
                return alloc_->GetFile()->Read<Page>(dummy_offset_);
            }
        }

        void WritePage() {
            if (curr_.index_ < alloc_->GetPagesCount() && curr_.index_ > 0) {
                curr_.WritePage(alloc_->GetFile(), alloc_->GetCr3());
            } else {
                alloc_->GetFile()->Write<Page>(curr_, dummy_offset_);
            }
        }
    };
    PageList(const std::shared_ptr<PageAllocator>& alloc, Offset dummy_offset)
        : alloc_(alloc), dummy_offset_(dummy_offset) {
    }

    void Unlink(PageIterator it) {
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
    }

    void LinkBefore(PageIterator other, PageIterator it) {
        auto prev = PageIterator(alloc_, other->previous_page_index_, dummy_offset_);
        it->next_page_index_ = other->index_;
        it->previous_page_index_ = prev->index_;
        prev->next_page_index_ = it->index_;
        other->previous_page_index_ = it->index_;
    }

    PageIterator begin() {
        return PageIterator(alloc_,
                            alloc_->GetFile()->Read<Page>(dummy_offset_).previous_page_index_,
                            dummy_offset_);
    }
    PageIterator end() {
        return PageIterator(alloc_, alloc_->GetFile()->Read<Page>(dummy_offset_).index_,
                            dummy_offset_);
    }
};
}  // namespace mem