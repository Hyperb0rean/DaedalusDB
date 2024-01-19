#pragma once
#include <list>

#include "mem.hpp"

namespace mem {

class PageAllocator : public std::enable_shared_from_this<PageAllocator> {

    Offset cr3_;
    size_t pages_count_;
    size_t freelist_index_;
    std::shared_ptr<mem::File> file_;
    Page current_metadata_page_;

    [[nodiscard]] Offset GetPageAddress(size_t index) {
        return cr3_ + index * kPageSize;
    }

    [[nodiscard]] size_t GetIndex(Offset offset) {
        return (offset - cr3_) / kPageSize;
    }

    [[nodiscard]] Page ReadPage(size_t index) {
        return file_->Read<Page>(GetPageAddress(index));
    }

    void WritePage(Page page) {
        file_->Write<Page>(page, GetPageAddress(page.index));
    }

public:
    class PageIterator {
        std::shared_ptr<mem::PageAllocator> alloc_;
        Page curr_;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Page;
        using difference_type = size_t;
        using pointer = Page*;
        using reference = Page&;

        PageIterator(const std::shared_ptr<mem::PageAllocator>& alloc, size_t index)
            : alloc_(alloc) {
            curr_ = alloc_->ReadPage(index);
        }
        PageIterator& operator++() {
            curr_ = alloc_->ReadPage(curr_.next_page_index);
            return *this;
        }
        PageIterator operator++(int) {
            auto temp = *this;
            curr_ = alloc_->ReadPage(curr_.next_page_index);
            return temp;
        }

        PageIterator& operator--() {
            curr_ = alloc_->ReadPage(curr_.previous_page_index);
            return *this;
        }
        PageIterator operator--(int) {
            auto temp = *this;
            curr_ = alloc_->ReadPage(curr_.previous_page_index);
            return temp;
        }

        Page& operator*() {
            return curr_;
        }
        Page* operator->() {
            return &curr_;
        }

        bool operator==(const PageIterator& other) const {
            return curr_.index == other.curr_.index;
        }
        bool operator!=(const PageIterator& other) const {
            return !(*this == other);
        }
    };

    [[nodiscard]] PageIterator GetFreeList() {
        return PageIterator(shared_from_this(), freelist_index_);
    }

    [[nodiscard]] size_t GetPagesCount() const {
        return pages_count_;
    }

    PageAllocator(std::shared_ptr<mem::File>& file, Offset cr3, size_t pages_count,
                  size_t freelist_index)
        : cr3_(cr3), pages_count_(pages_count), freelist_index_(freelist_index), file_(file) {
        WritePage(Page{PageType::kMetadata, 0, 0, 0, 0, 0});
    }

    void Unlink(PageIterator it) {
        if (it->next_page_index == it->previous_page_index) {
            return;
        }
        auto prev_it = PageIterator(shared_from_this(), it->previous_page_index);
        auto next_it = PageIterator(shared_from_this(), it->next_page_index);

        prev_it->next_page_index = it->next_page_index;
        next_it->previous_page_index = it->previous_page_index;

        it->previous_page_index = it->index;
        it->next_page_index = it->index;

        WritePage(*prev_it);
        WritePage(*next_it);
        WritePage(*it);
    }

    void LinkBefore(PageIterator base, PageIterator it) {
        auto prev_it = PageIterator(shared_from_this(), base->previous_page_index);
        it->next_page_index = base->index;
        it->previous_page_index = prev_it->previous_page_index;
        prev_it->next_page_index = it->index;
        base->previous_page_index = it->index;
        WritePage(*base);
        WritePage(*prev_it);
        WritePage(*it);
    }

    [[nodiscard]] PageIterator AllocatePage() {
        return PageIterator(shared_from_this(), freelist_index_);
    }

    void FreePage(size_t index) {
    }
};

}  // namespace mem