#pragma once

#include "const_node_storage.hpp"
#include "var_node_storage.hpp"

namespace db {

enum class OpenMode { kDefault, kRead, kWrite };

class Database {

    DECLARE_LOGGER;
    mem::Superblock superblock_;
    std::shared_ptr<mem::File> file_;
    std::shared_ptr<mem::PageAllocator> alloc_;
    std::shared_ptr<ClassStorage> class_storage_;

    void InitializeSuperblock(OpenMode mode) {
        switch (mode) {
            case OpenMode::kRead: {
                DEBUG("OpenMode: Read");
                superblock_.ReadSuperblock(file_);
            } break;
            case OpenMode::kDefault: {
                DEBUG("OpenMode: Default");
                try {
                    superblock_.ReadSuperblock(file_);
                    break;
                } catch (const error::StructureError& e) {
                    ERROR("Can't open file in Read mode, rewriting..");
                } catch (const error::BadArgument& e) {
                    ERROR("Can't open file in Read mode, rewriting..");
                }
            }
            case OpenMode::kWrite: {
                DEBUG("OpenMode: Write");
                file_->Clear();
                superblock_.InitSuperblock(file_);
            } break;
        }
    }

public:
    Database(const std::shared_ptr<mem::File>& file, OpenMode mode = OpenMode::kDefault,
             DEFAULT_LOGGER(logger))
        : LOGGER(logger), file_(file) {

        InitializeSuperblock(mode);

        alloc_ = std::make_shared<mem::PageAllocator>(file_, LOGGER);
        INFO("Allocator initialized");
        class_storage_ = std::make_shared<ClassStorage>(alloc_, LOGGER);
    }

    ~Database() {
        INFO("Closing database");
    };

    template <ts::ClassLike C>
    void AddClass(std::shared_ptr<C>& new_class) {
        class_storage_->AddClass(new_class);
    }

    template <ts::ClassLike C>
    void RemoveClass(std::shared_ptr<C>& new_class) {
        class_storage_->RemoveClass(new_class);
    }

    void PrintClasses() {
        auto& alloc = alloc_;
        class_storage_->VisitClasses([alloc](mem::ClassHeader class_header) -> void {
            ts::ClassObject class_object;
            class_object.Read(alloc->GetFile(),
                              mem::GetOffset(class_header.index_, class_header.free_offset_));
            std::cout << " [ " << class_header.index_ << " ] " << class_object.ToString()
                      << std::endl;
        });
    }

    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) void AddNode(std::shared_ptr<O> node) {
        if (node->GetClass()->Size().has_value()) {
            ConstantSizeNodeStorage(node->GetClass(), class_storage_, alloc_, LOGGER).AddNode(node);
        } else {
            VariableSizeNodeStorage(node->GetClass(), class_storage_, alloc_, LOGGER).AddNode(node);
        }
    }
};

}  // namespace db
