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
        file->Write<size_t>(count, GetOffset(index_, 2 * sizeof(Page) + sizeof(size_t)));
        return *this;
    }

    ClassHeader& WriteMagic(std::shared_ptr<File>& file, uint64_t count) {
        file->Write<uint64_t>(count, GetOffset(index_, 2 * sizeof(Page) + 2 * sizeof(size_t)));
        return *this;
    }

    ClassHeader& ReadClassHeader(std::shared_ptr<File>& file) {
        auto header = file->Read<ClassHeader>(this->GetPageAddress(kPagetableOffset));
        std::swap(header, *this);
        return *this;
    }
    ClassHeader& InitClassHeader(std::shared_ptr<File>& file, size_t size = 0) {
        this->type_ = PageType::kClassHeader;
        this->actual_size_ = size;
        this->first_free_ = sizeof(ClassHeader);
        node_list_sentinel_ = Page(kSentinelIndex);
        node_list_sentinel_.type_ = PageType::kSentinel;
        node_pages_count_ = 0;
        nodes_ = 0;
        magic_ = 0;
        file->Write<ClassHeader>(*this, this->GetPageAddress(kPagetableOffset));
        return *this;
    }
    ClassHeader& WriteClassHeader(std::shared_ptr<File>& file) {
        file->Write<ClassHeader>(*this, this->GetPageAddress(kPagetableOffset));
        return *this;
    }
};

}  // namespace mem