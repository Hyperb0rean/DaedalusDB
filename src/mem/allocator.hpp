#pragma once

#include <cstddef>

#include "logger.hpp"
#include "mem.hpp"
#include "page.hpp"
#include "pagelist.hpp"

namespace mem {

class PageAllocator {

private:
    DECLARE_LOGGER;
    size_t pages_count_;
    File::Ptr file_;
    PageList free_list_;

    static constexpr double kLoadFactor = 0.5;

    auto AllocateNewPage() -> PageIndex {
        if ((file_->GetSize() - kPagetableOffset) % kPageSize != 0) {
            ERROR("Filesize: ", file_->GetSize());
            throw error::StructureError("Unaligned file");
        }
        auto new_page_offset = file_->GetSize();

        DEBUG("Allocating page");
        DEBUG("Filesize: ", new_page_offset);

        file_->Extend(kPageSize);
        file_->Write<Page>(Page(pages_count_++), new_page_offset);
        file_->Write<size_t>(pages_count_, kPagesCountOffset);

        DEBUG("Successful Allocation");

        return pages_count_ - 1;
    }

    auto SwapPages(PageIndex first, PageIndex second) -> void {
        if (first > pages_count_ || second > pages_count_) {
            throw error::BadArgument("The page index exceedes pages count: " +
                                     std::to_string(pages_count_));
        }

        DEBUG("Swapping pages with indecies ", first, " ", second);

        auto first_data = file_->Read<PageData>(GetPageAddress(first));
        auto second_data = file_->Read<PageData>(GetPageAddress(second));

        std::swap(first_data.page_header.index_, second_data.page_header.index_);
        std::swap(first_data.page_header.next_page_index_,
                  second_data.page_header.next_page_index_);
        std::swap(first_data.page_header.previous_page_index_,
                  second_data.page_header.previous_page_index_);

        file_->Write<PageData>(first_data, GetPageAddress(second_data.page_header.index_));
        file_->Write<PageData>(second_data, GetPageAddress(first_data.page_header.index_));

        DEBUG("Successfully swaped");
    }

    // TODO: Need to do better compression but the problem that i need to know from which list
    // swapping occurs, but i can't. I'm tired
    auto Compression() -> void {
        auto rbegin = free_list_.RBegin();
        auto page_it = pages_count_ - 1;
        size_t count = 0;
        while (page_it != 0) {
            DEBUG("Page: ", ReadPage(Page(page_it), file_));
            if (ReadPage(Page(page_it), file_).type_ == PageType::kFree) {
                --pages_count_;
                ++count;
                free_list_.Unlink(page_it--);
            } else {
                break;
            }
        }
        DEBUG("TRUNCATE: ", count * kPageSize);
        file_->Truncate(count * kPageSize);
    }

public:
    using Ptr = util::Ptr<PageAllocator>;

    PageAllocator(mem::File::Ptr& file, DEFAULT_LOGGER(logger)) : LOGGER(logger), file_(file) {

        DEBUG("Free list count: ", file->Read<size_t>(kPagesCountOffset));
        pages_count_ = file_->Read<size_t>(kPagesCountOffset);

        DEBUG("Freelist sentinel offset: ", kFreeListSentinelOffset);
        DEBUG("Free list count: ", file->Read<size_t>(kFreePagesCountOffset));

        free_list_ = PageList("Free_List", file_, kFreeListSentinelOffset, LOGGER);

        INFO("FreeList initialized");
    }

    [[nodiscard]] auto GetPagesCount() const noexcept -> size_t {
        return pages_count_;
    }

    [[nodiscard]] auto GetFile() -> File::Ptr& {
        return file_;
    }

    auto AllocatePage() -> PageIndex {
        if (free_list_.IsEmpty()) {
            return AllocateNewPage();
        }
        auto index = free_list_.Back();
        free_list_.PopBack();
        return index;
    }

    auto FreePage(PageIndex index) -> void {
        if (free_list_.IteratorTo(index)->type_ == PageType::kFree) {
            throw error::RuntimeError("Double free");
        }
        WritePage(Page(index), file_);
        free_list_.PushBack(index);

        if (static_cast<double>(free_list_.GetPagesCount()) / pages_count_ > kLoadFactor) {
            DEBUG("Compression");
            Compression();
        }
    }
};

}  // namespace mem