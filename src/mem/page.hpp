#pragma once

#include "file.hpp"

namespace mem {

inline const size_t kPageSize = 4096;
enum class PageType { kClassHeader, kData, kFree, kMetadata };

using PageOffset = uint32_t;

class Page {
public:
    PageType type_;
    size_t index_;
    size_t actual_size_;
    PageOffset first_free_;
    size_t previous_page_index_;
    size_t next_page_index_;

    Page(size_t index)
        : type_(PageType::kFree),
          index_(index),
          actual_size_(0),
          first_free_(sizeof(Page)),
          previous_page_index_(index_),
          next_page_index_(index_) {
    }

    Page() : Page(0) {
    }

    [[nodiscard]] Offset GetPageAddress(Offset cr3) {
        return cr3 + index_ * kPageSize;
    }

    [[nodiscard]] Page ReadPage(const std::shared_ptr<File>& file, Offset cr3) {
        auto page = file->Read<Page>(GetPageAddress(cr3));
        type_ = page.type_;
        actual_size_ = page.actual_size_;
        first_free_ = page.first_free_;
        previous_page_index_ = page.previous_page_index_;
        next_page_index_ = page.next_page_index_;
        return *this;
    }

    void WritePage(const std::shared_ptr<File>& file, Offset cr3) {
        file->Write<Page>(*this, GetPageAddress(index_));
    }
};

[[nodiscard]] inline size_t GetIndex(Offset cr3, Offset offset) {
    return (offset - cr3) / kPageSize;
}

}  // namespace mem