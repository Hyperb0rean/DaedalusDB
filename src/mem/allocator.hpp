#pragma once

#include "mem.hpp"

namespace mem {

class PageAllocator : public std::enable_shared_from_this<PageAllocator> {

    Offset cr3_;
    size_t pages_count_;
    size_t freelist_index_;
    std::shared_ptr<mem::File> file_;
    size_t current_metadata_page_;

public:
    PageAllocator(std::shared_ptr<mem::File>& file, Offset cr3, size_t pages_count,
                  size_t freelist_index)
        : cr3_(cr3), pages_count_(pages_count), freelist_index_(freelist_index), file_(file) {
    }

    [[nodiscard]] size_t GetPagesCount() const {
        return pages_count_;
    }

    [[nodiscard]] Offset GetCr3() const {
        return cr3_;
    }

    [[nodiscard]] const std::shared_ptr<mem::File>& GetFile() const {
        return file_;
    }

    void FreePage(size_t index) {
    }
};

}  // namespace mem