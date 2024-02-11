#pragma once

#include <iostream>

#include "mem.hpp"

namespace mem {

class PageList {

    DECLARE_LOGGER;
    std::string name_;
    std::shared_ptr<File> file_;
    Offset sentinel_offset_;
    size_t pages_count_;

    void DecrementCount() {
        file_->Write<size_t>(--pages_count_, GetCountFromSentinel(sentinel_offset_));
        DEBUG(name_, " Decremented page count, current: ", pages_count_);
    }
    void IncrementCount() {
        file_->Write<size_t>(++pages_count_, GetCountFromSentinel(sentinel_offset_));
        DEBUG(name_, " Incremented page count, current: ", pages_count_);
    }

public:
    [[nodiscard]] size_t GetPagesCount() const {
        DEBUG(name_, " Pages in list: ", pages_count_);
        return pages_count_;
    }

    class PageIterator {
        std::shared_ptr<File> file_;
        Offset sentinel_offset_;
        Page curr_;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Page;
        using difference_type = size_t;
        using pointer = Page*;
        using reference = Page&;

        PageIterator(const std::shared_ptr<File>& file, PageIndex index, Offset sentinel_offset)
            : file_(file), sentinel_offset_(sentinel_offset) {
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
            if (index < kSentinelIndex) {
                return mem::ReadPage(Page(index), file_);
            } else {
                return file_->Read<Page>(sentinel_offset_);
            }
        }

        void WritePage() {
            if (curr_.index_ < kSentinelIndex) {
                mem::WritePage(curr_, file_);
            } else {
                file_->Write<Page>(curr_, sentinel_offset_);
            }
        }
    };

    PageList() {
    }

    PageList(std::string name, const std::shared_ptr<File>& file, Offset sentinel_offset,
             std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : LOGGER(logger), name_(std::move(name)), file_(file), sentinel_offset_(sentinel_offset) {
        pages_count_ = file_->Read<size_t>(GetCountFromSentinel(sentinel_offset));
    }

    void Unlink(PageIndex index) {
        auto it = PageIterator(file_, index, sentinel_offset_);
        if (it->next_page_index_ == it->index_ && it->previous_page_index_ == it->index_) {
            return;
        }

        DEBUG(name_, " Unlinking page ", index);

        auto prev = PageIterator(file_, it->previous_page_index_, sentinel_offset_);
        auto next = PageIterator(file_, it->next_page_index_, sentinel_offset_);
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
        DEBUG(name_, " Linking page ", index, " before ", other_index);

        auto it = PageIterator(file_, index, sentinel_offset_);
        auto other = PageIterator(file_, other_index, sentinel_offset_);
        auto prev = PageIterator(file_, other->previous_page_index_, sentinel_offset_);

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
        return PageIterator(file_, index, sentinel_offset_);
    }

    void PushBack(PageIndex index) {
        DEBUG(name_, " Push back");
        LinkBefore(IteratorTo(kSentinelIndex)->next_page_index_, index);
    }

    void PushFront(PageIndex index) {
        DEBUG(name_, " Push front");
        LinkBefore(kSentinelIndex, index);
    }

    void PopBack() {
        DEBUG(name_, " Pop back");
        Unlink(IteratorTo(kSentinelIndex)->next_page_index_);
    }
    void PopFront() {
        DEBUG(name_, " Pop front");
        Unlink(IteratorTo(kSentinelIndex)->previous_page_index_);
    }

    bool IsEmpty() const {
        return GetPagesCount() == 0;
    }

    PageIterator Begin() {
        return PageIterator(file_, file_->Read<Page>(sentinel_offset_).previous_page_index_,
                            sentinel_offset_);
    }
    PageIterator End() {
        return PageIterator(file_, file_->Read<Page>(sentinel_offset_).index_, sentinel_offset_);
    }

    PageIndex Front() {
        return file_->Read<Page>(sentinel_offset_).previous_page_index_;
    }

    PageIndex Back() {
        return file_->Read<Page>(sentinel_offset_).next_page_index_;
    }
};

PageList::PageIterator begin(PageList& list) {
    return list.Begin();
}

PageList::PageIterator end(PageList& list) {
    return list.End();
}

}  // namespace mem