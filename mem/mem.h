#include <memory>
#include <vector>

#include "file.h"

namespace mem {

class Superblock {
    size_t types_;

    struct TypeEntry {
        std::string label;
        Offset offset;
    };
    std::vector<TypeEntry> entries_;

public:
    Superblock(const std::unique_ptr<File>& file);
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
    TypeHeader(Offset start, const std::unique_ptr<File>& file);
};

}  // namespace mem