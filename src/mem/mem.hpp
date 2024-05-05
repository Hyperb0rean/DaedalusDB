#pragma once
#include <cstddef>

#include "file.hpp"
#include "page.hpp"

namespace mem {

using GlobalMagic = uint64_t;
constexpr inline GlobalMagic kMagic = 0xDEADBEEF;

// Constant offsets of some data in superblock for more precise changes
constexpr Offset kFreeListSentinelOffset = sizeof(GlobalMagic);
constexpr Offset kFreePagesCountOffset =
    kFreeListSentinelOffset + static_cast<Offset>(sizeof(Page));
constexpr Offset kPagesCountOffset = kFreePagesCountOffset + static_cast<Offset>(sizeof(Offset));
constexpr Offset kClassListSentinelOffset = kPagesCountOffset + static_cast<Offset>(sizeof(size_t));
constexpr Offset kClassListCount = kClassListSentinelOffset + static_cast<Offset>(sizeof(Page));

constexpr Offset kPagetableOffset = kClassListCount + static_cast<Offset>(sizeof(size_t));

constexpr PageIndex kSentinelIndex = SIZE_MAX;

class Superblock {
public:
    Page free_list_sentinel_;
    size_t free_pages_count_;
    size_t pages_count;
    Page class_list_sentinel_;
    size_t class_list_count_;

    auto CheckConsistency(File::Ptr& file) -> void {
        try {
            auto magic = file->Read<GlobalMagic>();
            if (magic != kMagic) {
                throw;
            }

        } catch (...) {
            throw error::StructureError("Can't open database from this file: " +
                                        std::string{file->GetFilename()});
        }
    }

    auto ReadSuperblock(File::Ptr& file) -> Superblock& {
        CheckConsistency(file);
        auto header = file->Read<Superblock>(sizeof(kMagic));
        std::swap(header, *this);
        return *this;
    }

    auto InitSuperblock(File::Ptr& file) -> Superblock& {
        file->Write<GlobalMagic>(kMagic);
        free_list_sentinel_ = Page(kSentinelIndex);
        free_list_sentinel_.type_ = PageType::kSentinel;
        free_pages_count_ = 0;
        pages_count = 0;
        class_list_sentinel_ = Page(kSentinelIndex);
        class_list_count_ = 0;
        class_list_sentinel_.type_ = PageType::kSentinel;

        file->Write<Superblock>(*this, sizeof(kMagic));
        return *this;
    }
    auto WriteSuperblock(File::Ptr& file) -> Superblock& {
        CheckConsistency(file);
        file->Write<Superblock>(*this, sizeof(kMagic));
        return *this;
    }
};

constexpr inline auto GetCountFromSentinel(Offset sentinel) -> Offset {
    return sentinel + static_cast<Offset>(sizeof(Page));
}

constexpr inline auto GetSentinelIndex(Offset sentinel) -> Offset {
    return sentinel + static_cast<Offset>(sizeof(PageType));
}

constexpr inline auto GetOffset(PageIndex index, PageOffset virt_offset) -> Offset {
    if (virt_offset >= mem::kPageSize) {
        throw error::BadArgument("Invalid virtual offset");
    }
    return kPagetableOffset + static_cast<Offset>(index) * mem::kPageSize + virt_offset;
}

constexpr inline auto GetIndex(Offset offset) -> PageIndex {
    return static_cast<PageIndex>((offset - kPagetableOffset) / kPageSize);
}

constexpr inline auto GetPageAddress(PageIndex index) -> Offset {
    return kPagetableOffset + static_cast<Offset>(index) * kPageSize;
}

using Magic = uint64_t;

class ClassHeader : public Page {
public:
    Page node_list_sentinel_;
    size_t node_pages_count_;
    size_t id_;
    Magic magic_;

    ClassHeader() : Page() {
        this->type_ = PageType::kClassHeader;
    }

    ClassHeader(PageIndex index) : Page(index) {
        this->type_ = PageType::kClassHeader;
    }

    auto GetNodeListSentinelOffset() -> Offset {
        return GetOffset(index_, sizeof(Page));
    }

    // Should think about structure alignment in 4 following methods

    auto WriteNodeId(File::Ptr& file, size_t count) -> ClassHeader& {
        id_ = count;
        file->Write<size_t>(id_, GetOffset(index_, 2 * sizeof(Page) + sizeof(size_t)));
        return *this;
    }

    auto WriteMagic(File::Ptr& file, Magic magic) -> ClassHeader& {
        magic_ = magic;
        file->Write<Magic>(magic_, GetOffset(index_, 2 * sizeof(Page) + 2 * sizeof(size_t)));
        return *this;
    }

    auto ReadNodeId(File::Ptr& file) -> ClassHeader& {
        id_ = file->Read<size_t>(GetOffset(index_, 2 * sizeof(Page) + sizeof(size_t)));
        return *this;
    }

    auto ReadMagic(File::Ptr& file) -> ClassHeader& {
        magic_ = file->Read<Magic>(GetOffset(index_, 2 * sizeof(Page) + 2 * sizeof(size_t)));
        return *this;
    }

    auto ReadClassHeader(File::Ptr& file) -> ClassHeader& {
        auto header = file->Read<ClassHeader>(GetPageAddress(index_));
        std::swap(header, *this);
        return *this;
    }
    auto InitClassHeader(File::Ptr& file, size_t size = 0) -> ClassHeader& {
        this->type_ = PageType::kClassHeader;
        this->initialized_offset_ = static_cast<PageOffset>(sizeof(ClassHeader) + size);
        this->free_offset_ = sizeof(ClassHeader);
        this->actual_size_ = size;
        node_list_sentinel_ = Page(kSentinelIndex);
        node_list_sentinel_.type_ = PageType::kSentinel;
        node_pages_count_ = 0;
        id_ = 0;
        magic_ = 0;
        file->Write<ClassHeader>(*this, GetPageAddress(index_));
        return *this;
    }
    auto WriteClassHeader(File::Ptr& file) -> ClassHeader& {
        file->Write<ClassHeader>(*this, GetPageAddress(index_));
        return *this;
    }
};

inline auto ReadPage(Page other, File::Ptr& file) -> Page {
    auto page = file->Read<Page>(GetPageAddress(other.index_));
    std::swap(page, other);
    return other;
}

inline auto WritePage(Page other, File::Ptr& file) -> Page {
    file->Write<Page>(other, GetPageAddress(other.index_));
    return other;
}

}  // namespace mem