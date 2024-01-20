#pragma once

#include "mem.hpp"

namespace mem {

class PageAllocator : public std::enable_shared_from_this<PageAllocator> {

    Offset cr3_;
    size_t pages_count_;
    std::shared_ptr<mem::File> file_;

public:
    PageAllocator(std::shared_ptr<mem::File>& file, Offset cr3, size_t pages_count)
        : cr3_(cr3), pages_count_(pages_count), file_(file) {
    }

    [[nodiscard]] size_t GetPagesCount() const {
        return pages_count_;
    }

    [[nodiscard]] Offset GetCr3() const {
        return cr3_;
    }

    [[nodiscard]] const std::shared_ptr<mem::File>& GetFile() const {
        return file_;
    }

    PageIndex AllocatePage() {
        if ((file_->GetSize() - cr3_) % kPageSize != 0) {
            throw error::StructureError("Unaligned file");
        }
        auto new_page_offset = file_->GetSize();
        file_->Extend(kPageSize);
        file_->Write<Page>(Page(pages_count_++), new_page_offset);
        file_->Write<size_t>(pages_count_, kPagesCountOffset);
    }

    void SwapPages(PageIndex first, PageIndex second) {
        if (first > pages_count_ || second > pages_count_) {
            throw error::BadArgument("The page index exceedes pages count: " +
                                     std::to_string(pages_count_));
        }

        auto first_data = file_->Read<PageData>(Page(first).GetPageAddress(cr3_));
        auto second_data = file_->Read<PageData>(Page(second).GetPageAddress(cr3_));

        std::swap(first_data.page_header.index_, second_data.page_header.index_);
        std::swap(first_data.page_header.next_page_index_,
                  second_data.page_header.next_page_index_);
        std::swap(first_data.page_header.previous_page_index_,
                  second_data.page_header.previous_page_index_);

        file_->Write<PageData>(first_data, first_data.page_header.GetPageAddress(cr3_));
        file_->Write<PageData>(second_data, second_data.page_header.GetPageAddress(cr3_));
    }

    void FreePage(PageIndex index) {
        // TODO
    }
};

}  // namespace mem