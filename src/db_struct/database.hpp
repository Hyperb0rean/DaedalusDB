#pragma once

#include <iterator>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "pattern.hpp"
#include "struct.hpp"
#include "val_node_storage.hpp"
#include "var_node_storage.hpp"

namespace db {

enum class OpenMode { kDefault, kRead, kWrite };

using ValNodeIterator = db::ValNodeStorage::NodeIterator;
using VarNodeIterator = db::VarNodeStorage::NodeIterator;

class Database {

    DECLARE_LOGGER;
    mem::Superblock superblock_;
    mem::File::Ptr file_;
    mem::PageAllocator::Ptr alloc_;
    ClassStorage::Ptr class_storage_;

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
    using Ptr = util::Ptr<Database>;

    Database(const mem::File::Ptr& file, OpenMode mode = OpenMode::kDefault, DEFAULT_LOGGER(logger))
        : LOGGER(logger), file_(file) {

        InitializeSuperblock(mode);

        alloc_ = util::MakePtr<mem::PageAllocator>(file_, LOGGER);
        INFO("Allocator initialized");
        class_storage_ = util::MakePtr<ClassStorage>(alloc_, LOGGER);
    }

    ~Database() {
        INFO("Closing database");
    };

    template <ts::ClassLike C>
    void AddClass(const util::Ptr<C>& new_class) {
        class_storage_->AddClass(new_class);
    }

    template <ts::ClassLike C>
    void RemoveClass(const util::Ptr<C>& node_class) {
        NodeStorage(node_class, class_storage_, alloc_, LOGGER).Drop();
        class_storage_->RemoveClass(node_class);
    }

    template <ts::ClassLike C>
    bool Contains(const util::Ptr<C>& node_class) {
        bool found = 0;
        std::string serialized = node_class->Serialize();
        class_storage_->VisitClasses([&found, &serialized](ts::Class::Ptr stored_class) {
            if (found) {
                return;
            }
            if (stored_class->Serialize() == serialized) {
                found = true;
            }
        });
        return found;
    }

    void PrintClasses(std::ostream& os = std::cout) {
        auto& alloc = alloc_;
        class_storage_->VisitClasses([alloc, &os](mem::ClassHeader class_header) -> void {
            ts::ClassObject class_object;
            class_object.Read(alloc->GetFile(),
                              mem::GetOffset(class_header.index_, class_header.free_offset_));
            os << " [ " << class_header.index_ << " ] " << class_object.ToString() << std::endl;
        });
    }

    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) void AddNode(util::Ptr<O> node) {
        if (node->GetClass()->Size().has_value()) {
            ValNodeStorage(node->GetClass(), class_storage_, alloc_, LOGGER).AddNode(node);
        } else {
            VarNodeStorage(node->GetClass(), class_storage_, alloc_, LOGGER).AddNode(node);
        }
    }

    template <ts::ClassLike C, typename Predicate>
    void RemoveNodesIf(const util::Ptr<C>& node_class, Predicate predicate) {
        if (node_class->Size().has_value()) {
            if constexpr (std::is_invocable_r_v<bool, Predicate, ValNodeIterator>) {
                ValNodeStorage(node_class, class_storage_, alloc_, LOGGER).RemoveNodesIf(predicate);
            } else {
                ERROR("Bad predicate");
            }

        } else {
            if constexpr (std::is_invocable_r_v<bool, Predicate, VarNodeIterator>) {
                VarNodeStorage(node_class, class_storage_, alloc_, LOGGER).RemoveNodesIf(predicate);
            } else {
                ERROR("Bad predicate");
            }
        }
    }

    // TODO: I'm thinking about implementing some sort of Java StreamAPI-like API in future it
    // requires additional entity Stream or Sequence that will manage several Iterator's and some
    // constraints on them. It will introduce possibilities to chain predicates, zip iterators and
    // do other functional stuff but currently i'll focus on must-have functionality. Maybe somebody
    // or future me will implement this.

    template <ts::ClassLike C, typename Predicate, typename Functor>
    void VisitNodes(const util::Ptr<C>& node_class, Predicate predicate, Functor functor) {
        if (node_class->Size().has_value()) {
            if constexpr (std::is_invocable_r_v<bool, Predicate, ValNodeIterator>) {
                ValNodeStorage(node_class, class_storage_, alloc_, LOGGER)
                    .VisitNodes(predicate, functor);
            } else {
                ERROR("Bad predicate");
            }

        } else {
            if constexpr (std::is_invocable_r_v<bool, Predicate, VarNodeIterator>) {
                VarNodeStorage(node_class, class_storage_, alloc_, LOGGER)
                    .VisitNodes(predicate, functor);
            } else {
                ERROR("Bad predicate");
            }
        }
    }

    template <ts::ClassLike C, typename Predicate>
    void PrintNodesIf(util::Ptr<C>& node_class, Predicate predicate, std::ostream& os = std::cout) {
        auto print = [&os](auto node) { os << node->ToString() << std::endl; };
        VisitNodes(node_class, predicate, print);
    }

    template <ts::ObjectLike O, typename Container, ts::ClassLike C, typename Predicate>
    void CollectNodesIf(util::Ptr<C>& node_class,
                        std::back_insert_iterator<Container> back_inserter, Predicate predicate) {

        auto insert = [&back_inserter](auto node) {
            *back_inserter++ = (node->template Data<O>());
        };
        VisitNodes(node_class, predicate, insert);
    }

    // Very heavy operation
    // Definitly needed review and rethinking
    std::unordered_map<ts::ObjectId, ts::Struct::Ptr> PatternMatch(Pattern::Ptr pattern) {
        std::unordered_map<ts::ObjectId, ts::Struct::Ptr> result{};
        auto relations = pattern->GetRelations();
        for (auto& [name, end] : relations) {

            auto pattern_result = PatternMatch(util::As<Pattern>(end.pattern));
            bool is_terminal = util::As<Pattern>(end.pattern)->GetRelations().empty();
            std::unordered_map<ts::ObjectId, mem::Offset> from_index;
            std::unordered_map<ts::ObjectId, mem::Offset> to_index;

            auto fill_from = [&from_index](auto node) {
                from_index.insert({node.Id(), node.GetRealOffset()});
            };
            auto fill_to = [&to_index](auto node) {
                to_index.insert({node.Id(), node.GetRealOffset()});
            };

            VisitNodes(end.relation->FromClass(), kAll, fill_from);
            VisitNodes(end.relation->ToClass(), kAll, fill_to);
            auto from_magic =
                mem::ClassHeader(class_storage_->FindClass(end.relation->FromClass()).value())
                    .ReadClassHeader(alloc_->GetFile())
                    .magic_;

            auto to_magic =
                mem::ClassHeader(class_storage_->FindClass(end.relation->ToClass()).value())
                    .ReadClassHeader(alloc_->GetFile())
                    .magic_;

            auto& file = alloc_->GetFile();
            auto structure_class = ts::NewClass<ts::StructClass>(
                end.relation->Name(), end.relation->FromClass(),
                is_terminal ? end.relation->ToClass() : pattern_result.begin()->second->GetClass());

            //  TODO: Manage lazy deletion

            auto merge = [&from_index, &to_index, &pattern_result, &result, &end, &file,
                          &from_magic, &to_magic, is_terminal,
                          &structure_class](auto relation_node) {
                auto from = relation_node->template Data<ts::Relation>()->FromId();
                auto to = relation_node->template Data<ts::Relation>()->ToId();
                auto from_node =
                    Node(from_magic, end.relation->FromClass(), file, from_index[from]);
                auto to_node = Node(to_magic, end.relation->ToClass(), file, to_index[to]);

                if (end.predicate_(from_node, to_node)) {
                    auto new_struct = util::MakePtr<ts::Struct>(structure_class);
                    if (is_terminal) {
                        new_struct->AddFieldValue(from_node.Data<ts::Object>());
                        new_struct->AddFieldValue(to_node.Data<ts::Object>());
                    } else {
                        new_struct->AddFieldValue(from_node.Data<ts::Object>());
                        new_struct->AddFieldValue(pattern_result[to]);
                    }
                    result.insert({from, new_struct});
                }
            };

            VisitNodes(end.relation, kAll, merge);
        }
        return result;
    }
};

}  // namespace db
