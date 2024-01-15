#include <memory>
#include <vector>

#include "../util/intrusive_list.h"
#include "file.h"
#include "page.h"

namespace mem {

struct FreeListNode {
    Page first_page;
    size_t count_pages;
    Offset previous;
    Offset next;
};

class Superblock {
    size_t types_;
    Offset cr3_;
    FreeListNode head_;

    struct TypeEntry {
        std::string label;
        Offset offset;
    };
    std::vector<TypeEntry> entries_;

public:
    void ReadSuperblock(const std::unique_ptr<File>& file);
    void WriteSuperblock(const std::unique_ptr<File>& file);
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
        this->type_ = PageType::kTypeHeader;
    }

    void ReadTypeHeader(Offset start, const std::unique_ptr<File>& file);
    void WriteTypeHeader(Offset start, const std::unique_ptr<File>& file);
};

}  // namespace mem