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
        Offset free_list_head_;
        size_t free_list_count_;
    } header_;

    struct TypeEntry {
        std::string label;
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

    struct NodeEntry {
        Offset offset;
        size_t size;
    };
    std::vector<NodeEntry> entries_;

public:
    TypeHeader() {
        this->type = PageType::kTypeHeader;
    }

    void ReadTypeHeader(Offset start, std::shared_ptr<File>& file);
    void WriteTypeHeader(Offset start, std::shared_ptr<File>& file);
};

}  // namespace mem