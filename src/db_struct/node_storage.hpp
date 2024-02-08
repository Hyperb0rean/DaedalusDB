#pragma once

#include "class_storage.hpp"

namespace db {

class NodeStorage {
private:
    DECLARE_LOGGER;
    std::shared_ptr<ts::Class> nodes_class_;
    std::shared_ptr<ClassStorage> class_storage_;
    std::shared_ptr<mem::PageAllocator> alloc_;
    mem::PageList data_page_list_;

public:
    template <ts::ClassLike C>
    NodeStorage(std::shared_ptr<C>& nodes_class, std::shared_ptr<ClassStorage>& class_storage,
                std::shared_ptr<mem::PageAllocator>& alloc,
                std::shared_ptr<util::Logger> logger = std::make_shared<util::EmptyLogger>())
        : LOGGER(logger), nodes_class_(nodes_class), class_storage_(class_storage), alloc_(alloc) {
        mem::PageIndex index;
        auto found = class_storage_->FindClass(nodes_class_);
        if (std::holds_alternative<std::monostate>(found)) {
            throw error::RuntimeError("No such class in class storage");
        } else if (std::holds_alternative<mem::PageIndex>(found)) {
            index = std::get<mem::PageIndex>(found);
        } else {
            index = std::get<ClassStorage::ClassCache::iterator>(found)->second;
        }

        data_page_list_ = mem::PageList(alloc_, mem::GetNodeListSentinel(index), LOGGER);
    }
};
}  // namespace db
