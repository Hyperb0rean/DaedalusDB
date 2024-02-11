#include <memory>
#include <vector>

#include "file.hpp"
#include "page.hpp"

namespace mem {
constexpr int64_t kMagic = 0xDEADBEEF;

// Constant offsets of some data in superblock for more precise changes
constexpr Offset kFreeListSentinelOffset = sizeof(kMagic);
constexpr Offset kFreePagesCountOffset = kFreeListSentinelOffset + sizeof(Page);
constexpr Offset kPagesCountOffset = kFreePagesCountOffset + sizeof(Offset);
constexpr Offset kClassListSentinelOffset = kPagesCountOffset + sizeof(size_t);
constexpr Offset kClassListCount = kClassListSentinelOffset + sizeof(Page);

constexpr Offset kPagetableOffset = kClassListCount + sizeof(size_t);

constexpr PageIndex kSentinelIndex = SIZE_MAX;

class Superblock {
public:
    Page free_list_sentinel_;
    size_t free_pages_count_;
    size_t pages_count;
    Page class_list_sentinel_;
    size_t class_list_count_;

    void CheckConsistency(std::shared_ptr<File>& file) {
        try {
            auto magic = file->Read<int64_t>();
            if (magic != kMagic) {
                throw error::StructureError("Can't open database from this file: " +
                                            file->GetFilename());
            }

        } catch (...) {
            throw error::StructureError("Can't open database from this file: " +
                                        file->GetFilename());
        }
    }

    Superblock& ReadSuperblock(std::shared_ptr<File>& file) {
        CheckConsistency(file);
        auto header = file->Read<Superblock>(sizeof(kMagic));
        std::swap(header, *this);
        return *this;
    }

    Superblock& InitSuperblock(std::shared_ptr<File>& file) {
        file->Write<int64_t>(kMagic);
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
    Superblock& WriteSuperblock(std::shared_ptr<File>& file) {
        CheckConsistency(file);
        file->Write<Superblock>(*this, sizeof(kMagic));
        return *this;
    }
};

constexpr inline Offset GetCountFromSentinel(Offset sentinel) {
    return sentinel + sizeof(Page);
}

constexpr inline Offset GetSentinelIndex(Offset sentinel) {
    return sentinel + sizeof(PageType);
}

constexpr inline Offset GetOffset(PageIndex index, PageOffset virt_offset) {
    return kPagetableOffset + index * mem::kPageSize + virt_offset;
}

constexpr inline size_t GetIndex(Offset offset) {
    return (offset - kPagetableOffset) / kPageSize;
}

constexpr inline Offset GetPageAddress(PageIndex index) {
    return kPagetableOffset + index * kPageSize;
}

class ClassHeader : public Page {
public:
    Page node_list_sentinel_;
    size_t node_pages_count_;
    size_t nodes_;
    uint64_t magic_;

    ClassHeader() : Page() {
        this->type_ = PageType::kClassHeader;
    }

    ClassHeader(PageIndex index) : Page(index) {
        this->type_ = PageType::kClassHeader;
    }

    Offset GetNodeListSentinelOffset() {
        return GetOffset(index_, sizeof(Page));
    }

    ClassHeader& WriteNodeCount(std::shared_ptr<File>& file, size_t count) {
        nodes_ = count;
        file->Write<size_t>(nodes_, GetOffset(index_, 2 * sizeof(Page) + sizeof(size_t)));
        return *this;
    }

    ClassHeader& WriteMagic(std::shared_ptr<File>& file, uint64_t magic) {
        magic_ = magic;
        file->Write<uint64_t>(magic_, GetOffset(index_, 2 * sizeof(Page) + 2 * sizeof(size_t)));
        return *this;
    }

    ClassHeader& ReadNodeCount(std::shared_ptr<File>& file) {
        nodes_ = file->Read<size_t>(GetOffset(index_, 2 * sizeof(Page) + sizeof(size_t)));
        return *this;
    }

    ClassHeader& ReadMagic(std::shared_ptr<File>& file) {
        magic_ = file->Read<uint64_t>(GetOffset(index_, 2 * sizeof(Page) + 2 * sizeof(size_t)));
        return *this;
    }

    ClassHeader& ReadClassHeader(std::shared_ptr<File>& file) {
        auto header = file->Read<ClassHeader>(GetPageAddress(index_));
        std::swap(header, *this);
        return *this;
    }
    ClassHeader& InitClassHeader(std::shared_ptr<File>& file, size_t size = 0) {
        this->type_ = PageType::kClassHeader;
        this->initialized_offset_ = sizeof(ClassHeader) + size;
        this->free_offset_ = sizeof(ClassHeader);
        node_list_sentinel_ = Page(kSentinelIndex);
        node_list_sentinel_.type_ = PageType::kSentinel;
        node_pages_count_ = 0;
        nodes_ = 0;
        magic_ = 0;
        file->Write<ClassHeader>(*this, GetPageAddress(index_));
        return *this;
    }
    ClassHeader& WriteClassHeader(std::shared_ptr<File>& file) {
        file->Write<ClassHeader>(*this, GetPageAddress(index_));
        return *this;
    }
};

Page ReadPage(Page other, std::shared_ptr<File>& file) {
    auto page = file->Read<Page>(GetPageAddress(other.index_));
    std::swap(page, other);
    return other;
}

Page WritePage(Page other, std::shared_ptr<File>& file) {
    file->Write<Page>(other, GetPageAddress(other.index_));
    return other;
}

Page WriteFreeOffset(Page other, std::shared_ptr<File>& file, PageOffset free_offset) {
    other.free_offset_ = free_offset;
    file->Write<PageOffset>(
        other.free_offset_,
        GetOffset(other.index_, sizeof(PageType) + sizeof(PageIndex) + sizeof(PageOffset)));
    return other;
}

Page ReadFreeOffset(Page other, std::shared_ptr<File>& file) {
    other.free_offset_ = file->Read<PageOffset>(
        GetOffset(other.index_, sizeof(PageType) + sizeof(PageIndex) + sizeof(PageOffset)));
    return other;
}

Page WriteInitOffset(Page other, std::shared_ptr<File>& file, PageOffset init_offset) {
    other.initialized_offset_ = init_offset;
    file->Write<uint64_t>(other.initialized_offset_,
                          GetOffset(other.index_, sizeof(PageType) + sizeof(PageIndex)));
    return other;
}

Page ReadInitOffset(Page other, std::shared_ptr<File>& file) {
    other.initialized_offset_ =
        file->Read<PageOffset>(GetOffset(other.index_, sizeof(PageType) + sizeof(PageIndex)));
    return other;
}

}  // namespace mem