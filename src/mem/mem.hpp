#include <memory>
#include <vector>

#include "file.hpp"
#include "page.hpp"

namespace mem {
constexpr int64_t kMagic = 0xDEADBEEF;

class Superblock {

    struct SuperblockHeader {
        Page free_list_sentinel_;
        size_t types_;
        Offset cr3_;
        size_t pages_count;
        size_t free_pages_count;
        size_t class_metadata_index;
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

class ClassHeader : public Page {
    Page type_list_sentinel_;
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