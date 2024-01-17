#pragma once
#include <list>

#include "mem.h"

namespace mem {

class PageAllocator : public std::enable_shared_from_this<PageAllocator> {

    Offset cr3_;
    size_t pages_count_;
    std::shared_ptr<mem::File> file_;
    Page current_metadata_page_;

    [[nodiscard]] Page ReadPage(size_t index) {
        return file_->Read<Page>(cr3_ + index * kPageSize);
    }

public:
    [[nodiscard]] Offset GetPageAddress(Page page) {
        return cr3_ + page.index * kPageSize;
    }

    [[nodiscard]] size_t GetIndex(Offset offset) {
        return (offset - cr3_) / kPageSize;
    }

    class PageIterator {
        std::shared_ptr<mem::PageAllocator> alloc_;
        Page curr_;

        void ToEnd() {
            if (curr_.index == curr_.next_page_index) {
                curr_.index = alloc_->pages_count_ + 1;
            }
        }

    public:
        PageIterator() {
        }
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

        const Page& operator*() {
            ToEnd();
            return curr_;
        }
        const Page* operator->() {
            ToEnd();
            return &curr_;
        }

        bool operator==(const PageIterator& other) const {
            return curr_.index == other.curr_.index;
        }
        bool operator!=(const PageIterator& other) const {
            return !(*this == other);
        }
    };

private:
    PageIterator free_list_;

public:
    [[nodiscard]] PageIterator GetFreeList() {
        return free_list_;
    }

    [[nodiscard]] size_t GetPagesCount() {
        return pages_count_;
    }

    PageAllocator(std::shared_ptr<mem::File>& file, Offset cr3, size_t pages_count)
        : cr3_(cr3), pages_count_(pages_count), file_(file) {
    }

    void InitFreeList(size_t free_list_head_index) {
        free_list_ = PageIterator(shared_from_this(), free_list_head_index);
    }
};

}  // namespace mem