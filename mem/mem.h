#include <memory>
#include <vector>

#include "file.h"

namespace mem {

class Superblock {
    size_t types_;
    size_t relations_;

    struct TypeEntry {
        std::string label;
        Offset offset;
    };
    std::vector<TypeEntry> entries_;

public:
    Superblock(const std::shared_ptr<File>& file);
};

class TypeHeader {
    size_t nodes_;
    size_t size;

    std::vector<Offset> entries_;

public:
    TypeHeader(const std::shared_ptr<File>& file);
};

}  // namespace mem