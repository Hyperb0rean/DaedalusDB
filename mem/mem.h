#include <memory>
#include <vector>

#include "page.h"

namespace mem {

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

class TypeHeader {
    size_t nodes_;
    size_t size;

    struct NodeEntry {
        Offset offset;
        size_t size;
    };
    std::vector<NodeEntry> entries_;

public:
    void ReadTypeHeader(Offset start, const std::unique_ptr<File>& file);
    void WriteTypeHeader(Offset start, const std::unique_ptr<File>& file);
};

}  // namespace mem