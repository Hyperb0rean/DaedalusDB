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

    void CheckConsistency(File::Ptr& file) {
        try {
            auto magic = file->Read<GlobalMagic>();
            if (magic != kMagic) {
                throw error::StructureError("Can't open database from this file: " +
                                            file->GetFilename());
            }

        } catch (...) {
            throw error::StructureError("Can't open database from this file: " +
                                        file->GetFilename());
        }
    }

    Superblock& ReadSuperblock(File::Ptr& file) {
        CheckConsistency(file);
        auto header = file->Read<Superblock>(sizeof(kMagic));
        std::swap(header, *this);
        return *this;
    }

    Superblock& InitSuperblock(File::Ptr& file) {
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
    Superblock& WriteSuperblock(File::Ptr& file) {
        CheckConsistency(file);
        file->Write<Superblock>(*this, sizeof(kMagic));
        return *this;
    }
};

constexpr inline Offset GetCountFromSentinel(Offset sentinel) {
    return sentinel + static_cast<Offset>(sizeof(Page));
}

constexpr inline Offset GetSentinelIndex(Offset sentinel) {
    return sentinel + static_cast<Offset>(sizeof(PageType));
}

constexpr inline Offset GetOffset(PageIndex index, PageOffset virt_offset) {
    if (virt_offset >= mem::kPageSize) {
        throw error::BadArgument("Invalid virtual offset");
    }
    return kPagetableOffset + static_cast<Offset>(index) * mem::kPageSize + virt_offset;
}

constexpr inline PageIndex GetIndex(Offset offset) {
    return static_cast<PageIndex>((offset - kPagetableOffset) / kPageSize);
}

constexpr inline Offset GetPageAddress(PageIndex index) {
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

    Offset GetNodeListSentinelOffset() {
        return GetOffset(index_, sizeof(Page));
    }

    // Should think about structure alignment in 4 following methods

    ClassHeader& WriteNodeId(File::Ptr& file, size_t count) {
        id_ = count;
        file->Write<size_t>(id_, GetOffset(index_, 2 * sizeof(Page) + sizeof(size_t)));
        return *this;
    }

    ClassHeader& WriteMagic(File::Ptr& file, Magic magic) {
        magic_ = magic;
        file->Write<Magic>(magic_, GetOffset(index_, 2 * sizeof(Page) + 2 * sizeof(size_t)));
        return *this;
    }

    ClassHeader& ReadNodeId(File::Ptr& file) {
        id_ = file->Read<size_t>(GetOffset(index_, 2 * sizeof(Page) + sizeof(size_t)));
        return *this;
    }

    ClassHeader& ReadMagic(File::Ptr& file) {
        magic_ = file->Read<Magic>(GetOffset(index_, 2 * sizeof(Page) + 2 * sizeof(size_t)));
        return *this;
    }

    ClassHeader& ReadClassHeader(File::Ptr& file) {
        auto header = file->Read<ClassHeader>(GetPageAddress(index_));
        std::swap(header, *this);
        return *this;
    }
    ClassHeader& InitClassHeader(File::Ptr& file, size_t size = 0) {
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
    ClassHeader& WriteClassHeader(File::Ptr& file) {
        file->Write<ClassHeader>(*this, GetPageAddress(index_));
        return *this;
    }
};

inline Page ReadPage(Page other, File::Ptr& file) {
    auto page = file->Read<Page>(GetPageAddress(other.index_));
    std::swap(page, other);
    return other;
}

inline Page WritePage(Page other, File::Ptr& file) {
    file->Write<Page>(other, GetPageAddress(other.index_));
    return other;
}

}  // namespace mem