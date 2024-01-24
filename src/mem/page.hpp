#pragma once

#include "file.hpp"

namespace mem {

inline const size_t kPageSize = 4096;
enum class PageType { kClassHeader, kData, kFree, kSentinel };

using PageOffset = uint32_t;
using PageIndex = size_t;

class Page {
public:
    PageType type_;
    PageIndex index_;
    size_t actual_size_;
    PageOffset first_free_;
    PageIndex previous_page_index_;
    PageIndex next_page_index_;

    Page(PageIndex index)
        : type_(PageType::kFree),
          index_(index),
          actual_size_(0),
          first_free_(sizeof(Page)),
          previous_page_index_(index_),
          next_page_index_(index_) {
    }

    Page() : Page(0) {
    }

    [[nodiscard]] Offset GetPageAddress(Offset pagetable_offset) {
        return pagetable_offset + index_ * kPageSize;
    }

    [[nodiscard]] Page ReadPage(const std::shared_ptr<File>& file, Offset pagetable_offset) {
        auto page = file->Read<Page>(GetPageAddress(pagetable_offset));
        std::swap(page, *this);
        return *this;
    }

    void WritePage(const std::shared_ptr<File>& file, Offset pagetable_offset) {
        file->Write<Page>(*this, GetPageAddress(pagetable_offset));
    }
};

[[nodiscard]] inline size_t GetIndex(Offset pagetable_offset, Offset offset) {
    return (offset - pagetable_offset) / kPageSize;
}

struct PageData {
    Page page_header;
    char bytes[kPageSize - sizeof(Page)];
};

}  // namespace mem