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
    PageOffset initialized_offset_;
    PageOffset free_offset_;
    PageIndex previous_page_index_;
    PageIndex next_page_index_;

    Page(PageIndex index)
        : type_(PageType::kFree),
          index_(index),
          initialized_offset_(sizeof(Page)),
          free_offset_(sizeof(Page)),
          previous_page_index_(index_),
          next_page_index_(index_) {
    }

    Page() : Page(0) {
    }
};

struct PageData {
    Page page_header;
    char bytes[kPageSize - sizeof(Page)];
};

}  // namespace mem