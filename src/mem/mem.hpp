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
constexpr Offset kClassMetadataOffset = kTypesCountOffset + sizeof(size_t);

class Superblock {

    struct SuperblockHeader {
        Page free_list_sentinel_;
        size_t free_pages_count_;
        Offset cr3_;
        size_t pages_count;
        size_t types_count_;
        PageIndex class_metadata_index;
    } header_;

    void CheckConsistency(std::shared_ptr<File>& file) {
        auto magic = file->Read<int64_t>();
        if (magic != kMagic) {
            throw error::StructureError("Can't open database from this file: " +
                                        file->GetFilename());
        }
    }

public:
    void ReadSuperblock(std::shared_ptr<File>& file) {
        CheckConsistency(file);
        header_ = file->Read<SuperblockHeader>(sizeof(kMagic));
    }
    void WriteSuperblock(std::shared_ptr<File>& file);
};

constexpr inline Offset GetCountFromSentinel(Offset sentinel) {
    return sentinel + sizeof(Page);
}

constexpr inline Offset GetSentinelIndex(Offset sentinel) {
    return sentinel + sizeof(PageType);
}

class ClassHeader : public Page {
    Page type_list_sentinel_;
    size_t type_pages_count_;
    size_t nodes_;
    std::string serialized_class_;

public:
    ClassHeader() {
        this->type_ = PageType::kClassHeader;
    }

    void ReadClassHeader(Offset start, std::shared_ptr<File>& file);
    void WriteClassHeader(Offset start, std::shared_ptr<File>& file);
};

}  // namespace mem