#pragma once

#include "file.h"

namespace mem {

inline const size_t kPageSize = 4096;
enum class PageType { kTypeHeader, kData, kFree, kTypeMetadata };

struct Page {
    PageType type;
    size_t index;
    size_t actual_size;
    Offset first_free;
    size_t previous_page_index;
    size_t next_page_index;
};

}  // namespace mem