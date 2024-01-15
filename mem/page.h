#pragma once

#include <unistd.h>

namespace mem {
using Offset = off_t;

const size_t kPageSize = 4096;
enum class PageType { kTypeHeader, kData, kAllocatorMetadata };

class Page {
protected:
    PageType type_;
    size_t index_;
    size_t actual_size_;
    Offset first_free_;
};
}  // namespace mem