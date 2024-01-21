#include <memory>
#include <vector>

#include "file.hpp"
#include "page.hpp"

namespace mem {
constexpr int64_t kMagic = 0xDEADBEEF;

// Constant offsets of some data in superblock for more precise changes
constexpr Offset kFreeListSentinelOffset = 0;
constexpr Offset kFreePagesCountOffset = kFreeListSentinelOffset + sizeof(Page);
constexpr Offset kCr3Offset = kFreePagesCountOffset + sizeof(size_t);
constexpr Offset kPagesCountOffset = kCr3Offset + sizeof(Offset);
constexpr Offset kTypesCountOffset = kPagesCountOffset + sizeof(size_t);
constexpr Offset kClassListSentinelOffset = kTypesCountOffset + sizeof(size_t);
constexpr Offset kClassListCount = kClassListSentinelOffset + sizeof(Page);

constexpr PageIndex kDummyIndex = SIZE_MAX;

class Superblock {
public:
    Page free_list_sentinel_;
    size_t free_pages_count_;
    Offset cr3_;
    size_t pages_count;
    size_t types_count_;
    Page class_list_sentinel_;
    size_t class_list_count_;

    void CheckConsistency(std::shared_ptr<File>& file) {
        auto magic = file->Read<int64_t>();
        if (magic != kMagic) {
            throw error::StructureError("Can't open database from this file: " +
                                        file->GetFilename());
        }
    }

    void ReadSuperblock(std::shared_ptr<File>& file) {
        CheckConsistency(file);
        auto header = file->Read<Superblock>(sizeof(kMagic));
        std::swap(header, *this);
    }

    void InitSuperblock(std::shared_ptr<File>& file) {
        file->Write<int64_t>(kMagic);
        free_list_sentinel_ = Page(kDummyIndex);
        free_list_sentinel_.type_ = PageType::kSentinel;
        cr3_ = sizeof(kMagic) + sizeof(Superblock);
        pages_count = 0;
        types_count_ = 0;
        class_list_sentinel_ = Page(kDummyIndex);
        class_list_count_ = 0;
        class_list_sentinel_.type_ = PageType::kSentinel;

        file->Write<Superblock>(*this, sizeof(kMagic));
    }
    void WriteSuperblock(std::shared_ptr<File>& file) {
        CheckConsistency(file);
        file->Write<Superblock>(*this, sizeof(kMagic));
    }
};

constexpr inline Offset GetCountFromSentinel(Offset sentinel) {
    return sentinel + sizeof(Page);
}

constexpr inline Offset GetSentinelIndex(Offset sentinel) {
    return sentinel + sizeof(PageType);
}

class ClassHeader : public Page {
public:
    Page node_list_sentinel_;
    size_t node_pages_count_;
    size_t nodes_;

    ClassHeader() : Page() {
        this->type_ = PageType::kClassHeader;
    }

    ClassHeader(PageIndex index) : Page(index) {
        this->type_ = PageType::kClassHeader;
    }

    void ReadClassHeader(Offset cr3, std::shared_ptr<File>& file) {
        auto header = file->Read<ClassHeader>(this->GetPageAddress(cr3));
        std::swap(header, *this);
    }
    void InitClassHeader(Offset cr3, std::shared_ptr<File>& file) {
        this->type_ = PageType::kClassHeader;
        this->actual_size_ = 0;
        this->first_free_ = sizeof(ClassHeader);
        node_list_sentinel_ = Page(kDummyIndex);
        node_list_sentinel_.type_ = PageType::kSentinel;
        node_pages_count_ = 0;
        nodes_ = 0;
        file->Write<ClassHeader>(*this, this->GetPageAddress(cr3));
    }
    void WriteClassHeader(Offset cr3, std::shared_ptr<File>& file) {
        file->Write<ClassHeader>(*this, this->GetPageAddress(cr3));
    }
};

}  // namespace mem