#pragma once

#include "file.h"

namespace mem {

const size_t kPageSize = 4096;
enum class PageType { kTypeHeader, kData, kAllocatorMetadata };

struct Page {
    PageType type;
    size_t index;
    size_t actual_size;
    Offset first_free;
};

class PageRange {
    class PageIterator {
        Offset cr3_;
        size_t from_;
        size_t to_;
        Page curr_;
        std::shared_ptr<mem::File> file_;

        void ReadCurrentPage() {
            curr_ = file_->Read<Page>(cr3_ + from_ * kPageSize);
        }

    public:
        PageIterator(std::shared_ptr<mem::File>& file, size_t from, size_t to, Offset cr3)
            : cr3_(cr3), from_(from), to_(to), file_(file) {
        }
        PageIterator& operator++() {
            ++from_;
            return *this;
        }
        PageIterator operator++(int) {
            auto temp = *this;
            ++from_;
            return temp;
        }

        PageIterator& operator--() {
            --from_;
            return *this;
        }
        PageIterator operator--(int) {
            auto temp = *this;
            --from_;
            return temp;
        }

        const Page& operator*() {
            ReadCurrentPage();
            return curr_;
        }
        const Page* operator->() {
            ReadCurrentPage();
            return &curr_;
        }

        bool operator==(const PageIterator& other) const {
            return from_ == other.from_ && to_ == other.to_;
        }
        bool operator!=(const PageIterator& other) const {
            return !(*this == other);
        }
    };

    Offset cr3_;
    size_t from_;
    size_t to_;
    std::shared_ptr<mem::File> file_;

public:
    PageRange(size_t from, size_t to) : from_(from), to_(to) {
    }
    void SetSource(Offset cr3, const std::shared_ptr<mem::File>& file) {
        cr3_ = cr3;
        file_ = file;
    }

    [[nodiscard]] PageIterator begin() {
        return PageIterator(file_, from_, to_, cr3_);
    }
    [[nodiscard]] PageIterator end() {
        return PageIterator(file_, to_, to_, cr3_);
    }
};
}  // namespace mem