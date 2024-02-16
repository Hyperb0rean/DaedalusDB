#pragma once

#include "node_storage.hpp"

namespace db {
class VarNodeStorage : public NodeStorage {
public:
    template <ts::ClassLike C>
    VarNodeStorage(std::shared_ptr<C> nodes_class, std::shared_ptr<ClassStorage>& class_storage,
                   std::shared_ptr<mem::PageAllocator>& alloc, DEFAULT_LOGGER(logger))
        : NodeStorage(nodes_class, class_storage, alloc, logger) {
    }

    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) void AddNode(std::shared_ptr<O>& node) {
    }
};
}  // namespace db