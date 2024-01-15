#include <memory>
#include <vector>

#include "file.h"
#include "page.h"

namespace mem {

class Superblock {
    size_t types_;
    Offset cr3_;
    Offset free_list_head_;
    size_t free_list_count_;

    struct TypeEntry {
        std::string label;
        Offset offset;
    };
    std::vector<TypeEntry> entries_;

public:
    void ReadSuperblock(std::shared_ptr<File>& file);
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