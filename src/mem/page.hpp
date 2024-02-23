#pragma once

#include <ostream>

#include "file.hpp"

namespace mem {
inline const Offset kPageSize = 4096;

enum class PageType { kClassHeader, kData, kFree, kSentinel };

constexpr inline std::string_view PageTypeToString(PageType type) {
    switch (type) {
        case PageType::kClassHeader:
            return "Class Header";
        case PageType::kData:
            return "Data";
        case PageType::kFree:
            return "Free";
        case PageType::kSentinel:
            return "Sentinel";
        default:
            return "";
    }
}

using PageOffset = uint32_t;
using PageIndex = size_t;

class Page {
public:
    PageType type_;
    PageIndex index_;
    PageOffset initialized_offset_;
    PageOffset free_offset_;
    size_t actual_size_;
    PageIndex previous_page_index_;
    PageIndex next_page_index_;

    Page(PageIndex index)
        : type_(PageType::kFree),
          index_(index),
          initialized_offset_(sizeof(Page)),
          free_offset_(sizeof(Page)),
          actual_size_(0),
          previous_page_index_(index_),
          next_page_index_(index_) {
    }

    Page() : Page(0) {
    }
    friend std::ostream& operator<<(std::ostream& os, const Page& page) {
        return os << " [ " << page.index_ << " ] type: " << PageTypeToString(page.type_)
                  << ", init: " << page.initialized_offset_ << ", free: " << page.free_offset_
                  << ", size: " << page.actual_size_ << ", prev: " << page.previous_page_index_
                  << ", next: " << page.next_page_index_;
    }
};

struct PageData {
    Page page_header;
    char bytes[kPageSize - sizeof(Page)];
};

}  // namespace mem