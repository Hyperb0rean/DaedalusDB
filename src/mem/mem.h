#include <memory>
#include <vector>

#include "file.h"
#include "page.h"

namespace mem {
inline const int64_t kMagic = 0xDEADBEEF;

class Superblock {

    struct SuperblockHeader {
        size_t types_;
        Offset cr3_;
        size_t pages_count;
        size_t free_list_head_index_;
    } header_;

    struct TypeEntry {
        Offset offset;
    };
    std::vector<TypeEntry> entries_;

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

class TypeHeader : public Page {
    size_t nodes_;
    Page current_page_;
    Page first_page_;

public:
    TypeHeader() {
        this->type = PageType::kTypeHeader;
    }

    void ReadTypeHeader(Offset start, std::shared_ptr<File>& file);
    void WriteTypeHeader(Offset start, std::shared_ptr<File>& file);
};

}  // namespace mem