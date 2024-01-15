#pragma once
#include "mem.h"

namespace mem {

class PageAllocator {
    Offset cr3_;
    FreeListNode head_;
    Page current_metadata_page_;

public:
    void AllocatePageRange(size_t size) {
    }
};

}  // namespace mem