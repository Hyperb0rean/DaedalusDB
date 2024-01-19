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

        [[nodiscard]] Page ReadPage(size_t index) {
            if (index < alloc_->GetPagesCount()) {
                return Page(index).ReadPage(alloc_->GetFile(), alloc_->GetCr3());
            } else {
                return alloc_->GetFile()->Read<Page>(dummy_offset_);
            }
        }

        void WritePage() {
            if (curr_.index_ < alloc_->GetPagesCount()) {
                curr_.WritePage(alloc_->GetFile(), alloc_->GetCr3());
            } else {
                alloc_->GetFile()->Write<Page>(curr_, dummy_offset_);
            }
        }
    };
    PageList(const std::shared_ptr<PageAllocator>& alloc, Offset dummy_offset)
        : alloc_(alloc), dummy_offset_(dummy_offset) {
    }

    void Unlink(size_t index) {
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

        it.Write();
        prev.Write();
        next.Write();
    }

    void LinkBefore(size_t other_index, size_t index) {
        auto it = PageIterator(alloc_, index, dummy_offset_);
        auto other = PageIterator(alloc_, other_index, dummy_offset_);
        auto prev = PageIterator(alloc_, other->previous_page_index_, dummy_offset_);
        it->next_page_index_ = other->index_;
        it->previous_page_index_ = prev->index_;
        prev->next_page_index_ = it->index_;
        other->previous_page_index_ = it->index_;

        it.Write();
        other.Write();
        prev.Write();
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