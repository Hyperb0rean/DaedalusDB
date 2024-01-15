#pragma once
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
    std::vector<FreeListNode> free_list_;
    std::unique_ptr<mem::File> file_;
    Page current_metadata_page_;

public:
    PageAllocator(Offset cr3, Offset free_list_head, size_t free_list_count) : cr3_(cr3) {
        for (size_t i = 0; i < free_list_count; ++i) {
            free_list_.push_back(file_->Read<FreeListNode>(free_list_head));
            free_list_head = free_list_.back().next;
        }
    }

    [[nodiscard]] Offset AllocatePageRange(size_t count) {
    }
    void DeallocatePage(Offset page) {
    }
};

}  // namespace mem