#pragma once
#include <list>

#include "mem.h"

namespace mem {

struct FreeListNode {
    Page first_page;
    size_t count_pages;
    Offset previous;
    Offset next;
};

class PageAllocator {
    Offset cr3_;
    size_t pages_count_;
    std::shared_ptr<mem::File> file_;
    std::list<FreeListNode> free_list_;
    Page current_metadata_page_;

public:
    PageAllocator(std::shared_ptr<mem::File>& file, Offset cr3, size_t pages_count,
                  Offset free_list_head, size_t free_list_count)
        : cr3_(cr3), pages_count_(pages_count), file_(file) {
        for (size_t i = 0; i < free_list_count; ++i) {
            free_list_.push_back(file_->Read<FreeListNode>(free_list_head));
            free_list_head = free_list_.back().next;
        }
    }

    [[nodiscard]] PageRange GetPageRange(size_t from, size_t to) const {
        auto range = PageRange(from, to);
        range.SetSource(cr3_, file_);
        return range;
    }

    [[nodiscard]] Offset GetPageAddress(Page page) {
        return cr3_ + page.index * kPageSize;
    }

    [[nodiscard]] size_t GetIndex(Offset offset) {
        return (offset - cr3_) / kPageSize;
    }

    [[nodiscard]] PageRange AllocatePageRange(size_t count) {
        auto page_it = free_list_.end();
        for (auto it = free_list_.begin(); it != free_list_.end(); ++it) {
            if (it->count_pages >= count) {
                page_it = it;
                break;
            }
        }
        if (page_it != free_list_.end()) {
            free_list_.erase(page_it);
            return GetPageRange(page_it->first_page.index, page_it->first_page.index + count);
        } else {
            pages_count_ += count;
            return GetPageRange(pages_count_ - count, pages_count_);
        }
    }
    void DeallocatePageRange(PageRange range) {
        // Merge
    }
};

}  // namespace mem