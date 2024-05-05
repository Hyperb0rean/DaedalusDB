#pragma once
#include <cstddef>

#include "mem.hpp"
#include "page.hpp"

namespace mem {

class PageList {

    DECLARE_LOGGER;
    std::string name_;
    File::Ptr file_;
    Offset sentinel_offset_;
    size_t pages_count_;

    auto DecrementCount() -> void {
        file_->Write<size_t>(--pages_count_, GetCountFromSentinel(sentinel_offset_));
        DEBUG(name_, " Decremented page count, current: ", pages_count_);
    }
    auto IncrementCount() -> void {
        file_->Write<size_t>(++pages_count_, GetCountFromSentinel(sentinel_offset_));
        DEBUG(name_, " Incremented page count, current: ", pages_count_);
    }

public:
    [[nodiscard]] auto GetPagesCount() const noexcept -> size_t {
        return pages_count_;
    }

    class PageIterator {
        File::Ptr file_;
        Offset sentinel_offset_;
        Page curr_;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Page;
        using difference_type = size_t;
        using pointer = Page*;
        using reference = Page&;

        PageIterator(File::Ptr& file, PageIndex index, Offset sentinel_offset)
            : file_(file), sentinel_offset_(sentinel_offset) {
            curr_ = ReadPage(index);
        }
        auto operator++() -> PageIterator& {
            curr_ = ReadPage(curr_.previous_page_index_);
            return *this;
        }
        auto operator++(int) -> PageIterator {
            auto temp = *this;
            curr_ = ReadPage(curr_.previous_page_index_);
            return temp;
        }

        auto operator--() -> PageIterator& {
            curr_ = ReadPage(curr_.next_page_index_);
            return *this;
        }
        auto operator--(int) -> PageIterator {
            auto temp = *this;
            curr_ = ReadPage(curr_.next_page_index_);
            return temp;
        }

        auto operator*() noexcept -> reference {
            return curr_;
        }
        auto operator->() noexcept -> pointer {
            return &curr_;
        }

        auto operator==(const PageIterator& other) const noexcept -> bool {
            return curr_.index_ == other.curr_.index_;
        }
        auto operator!=(const PageIterator& other) const noexcept -> bool {
            return !(*this == other);
        }

        [[nodiscard]] auto Index() const noexcept -> PageIndex {
            return curr_.index_;
        }

        [[nodiscard]] auto ReadPage(PageIndex index) -> Page {
            if (index < kSentinelIndex) {
                return mem::ReadPage(Page(index), file_);
            } else {
                return file_->Read<Page>(sentinel_offset_);
            }
        }

        auto WritePage() -> void {
            if (curr_.index_ < kSentinelIndex) {
                mem::WritePage(curr_, file_);
            } else {
                file_->Write<Page>(curr_, sentinel_offset_);
            }
        }
    };

    PageList() noexcept {
    }

    PageList(std::string name, File::Ptr& file, Offset sentinel_offset, DEFAULT_LOGGER(logger))
        : LOGGER(logger), name_(std::move(name)), file_(file), sentinel_offset_(sentinel_offset) {
        pages_count_ = file_->Read<size_t>(GetCountFromSentinel(sentinel_offset));
    }

    auto Unlink(PageIndex index) -> void {
        auto it = PageIterator(file_, index, sentinel_offset_);
        if (it->next_page_index_ == it->index_ && it->previous_page_index_ == it->index_) {
            return;
        }

        DEBUG(name_, " Unlinking page ", index);

        auto prev = PageIterator(file_, it->previous_page_index_, sentinel_offset_);
        auto next = PageIterator(file_, it->next_page_index_, sentinel_offset_);
        prev->next_page_index_ = next->index_;
        next->previous_page_index_ = prev->index_;
        it->previous_page_index_ = it->index_;
        it->next_page_index_ = it->index_;

        if (GetPagesCount() == 1) {
            next->next_page_index_ = next->index_;
            next->previous_page_index_ = next->index_;
        }

        it.WritePage();
        prev.WritePage();
        next.WritePage();
        DecrementCount();
    }

    auto LinkBefore(PageIndex other_index, PageIndex index) -> void {
        // other_index must be from list, index must not be from list
        DEBUG(name_, " Linking page ", index, " before ", other_index);

        auto it = PageIterator(file_, index, sentinel_offset_);
        auto other = PageIterator(file_, other_index, sentinel_offset_);
        auto prev = PageIterator(file_, other->previous_page_index_, sentinel_offset_);

        it->next_page_index_ = other->index_;
        it->previous_page_index_ = prev->index_;
        prev->next_page_index_ = it->index_;
        other->previous_page_index_ = it->index_;

        if (GetPagesCount() == 0) {
            other->next_page_index_ = it->index_;
        }

        it.WritePage();
        prev.WritePage();
        other.WritePage();

        IncrementCount();
    }

    // Index must be in the list
    auto IteratorTo(PageIndex index) -> PageIterator {
        return PageIterator(file_, index, sentinel_offset_);
    }

    auto PushBack(PageIndex index) -> void {
        DEBUG(name_, " Push back");
        LinkBefore(IteratorTo(kSentinelIndex)->next_page_index_, index);
    }

    auto PushFront(PageIndex index) -> void {
        DEBUG(name_, " Push front");
        LinkBefore(kSentinelIndex, index);
    }

    auto PopBack() -> void {
        DEBUG(name_, " Pop back");
        Unlink(IteratorTo(kSentinelIndex)->next_page_index_);
    }
    auto PopFront() -> void {
        DEBUG(name_, " Pop front");
        Unlink(IteratorTo(kSentinelIndex)->previous_page_index_);
    }

    auto IsEmpty() const -> bool {
        return GetPagesCount() == 0;
    }

    auto Begin() -> PageIterator {
        return PageIterator(file_, file_->Read<Page>(sentinel_offset_).previous_page_index_,
                            sentinel_offset_);
    }
    auto End() -> PageIterator {
        return PageIterator(file_, file_->Read<Page>(sentinel_offset_).index_, sentinel_offset_);
    }
    auto RBegin() -> PageIterator {
        return PageIterator(file_, file_->Read<Page>(sentinel_offset_).next_page_index_,
                            sentinel_offset_);
    }

    auto Front() -> PageIndex {
        return file_->Read<Page>(sentinel_offset_).previous_page_index_;
    }

    auto Back() -> PageIndex {
        return file_->Read<Page>(sentinel_offset_).next_page_index_;
    }
};

inline auto begin(PageList& list) -> PageList::PageIterator {
    return list.Begin();
}

inline auto end(PageList& list) -> PageList::PageIterator {
    return list.End();
}

}  // namespace mem