#pragma once

#include "pagelist.hpp"

namespace mem {

class PageAllocator {

private:
    size_t pages_count_;
    std::shared_ptr<File> file_;
    PageList free_list_;
    std::shared_ptr<util::Logger> logger_;

    PageIndex AllocateNewPage() {
        if ((file_->GetSize() - kPagetableOffset) % kPageSize != 0) {
            ERROR("Filesize: " + std::to_string(file_->GetSize()));
            throw error::StructureError("Unaligned file");
        }
        auto new_page_offset = file_->GetSize();

        DEBUG("Allocating page");
        DEBUG("Filesize: " + std::to_string(new_page_offset));

        file_->Extend(kPageSize);
        file_->Write<Page>(Page(pages_count_++), new_page_offset);
        file_->Write<size_t>(pages_count_, kPagesCountOffset);

        DEBUG("Successful Allocation");

        return pages_count_ - 1;
    }

    void SwapPages(PageIndex first, PageIndex second) {
        if (first > pages_count_ || second > pages_count_) {
            throw error::BadArgument("The page index exceedes pages count: " +
                                     std::to_string(pages_count_));
        }

        DEBUG("Swapping pages with indecies" + std::to_string(first) + " " +
              std::to_string(second));

        auto first_data = file_->Read<PageData>(Page(first).GetPageAddress(kPagetableOffset));
        auto second_data = file_->Read<PageData>(Page(second).GetPageAddress(kPagetableOffset));

        std::swap(first_data.page_header.index_, second_data.page_header.index_);
        std::swap(first_data.page_header.next_page_index_,
                  second_data.page_header.next_page_index_);
        std::swap(first_data.page_header.previous_page_index_,
                  second_data.page_header.previous_page_index_);

        file_->Write<PageData>(first_data, first_data.page_header.GetPageAddress(kPagetableOffset));
        file_->Write<PageData>(second_data,
                               second_data.page_header.GetPageAddress(kPagetableOffset));

        DEBUG("Successfully swaped");
    }

public:
    PageAllocator(std::shared_ptr<mem::File>& file,
                  std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : file_(file), logger_(logger) {

        DEBUG("Free list count: " + std::to_string(file->Read<size_t>(kPagesCountOffset)));
        pages_count_ = file_->Read<size_t>(kPagesCountOffset);

        DEBUG("Freelist sentinel offset: " + std::to_string(kFreeListSentinelOffset));
        DEBUG("Free list count: " + std::to_string(file->Read<size_t>(kFreePagesCountOffset)));

        free_list_ = PageList(file_, kFreeListSentinelOffset, logger_);

        INFO("FreeList initialized");
    }

    [[nodiscard]] size_t GetPagesCount() const {
        return pages_count_;
    }

    [[nodiscard]] std::shared_ptr<mem::File>& GetFile() {
        return file_;
    }

    mem::PageIndex AllocatePage() {
        if (free_list_.IsEmpty()) {
            return AllocateNewPage();
        }
        auto index = free_list_.Back();
        free_list_.PopBack();
        return index;
    }

    void FreePage(mem::PageIndex index) {
        if (free_list_.IteratorTo(index)->type_ == mem::PageType::kFree) {
            throw error::RuntimeError("Double free");
        }
        free_list_.PushBack(index);

        // Rearranding list;
    }
};

}  // namespace mem