#pragma once

#include <iterator>
#include <optional>
#include <set>
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

    auto InitializeSuperblock(OpenMode mode) -> void {
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

    // Probably should introduce new entity to properly match or not match cycles, but the topics
    // need more time to investigate
    struct SubPatternResult {
        ts::ObjectId from;
        // Set but not unordered set to correct operator==
        std::set<ts::ObjectId> to;
        ts::Struct::Ptr value;
    };
    using PatterMatchResultImpl = std::vector<SubPatternResult>;

    auto IsSetNew(std::set<ts::ObjectId>& subpattern_set, ts::ObjectId to,
                  std::vector<std::set<ts::ObjectId>>& banned) -> bool {
        if (subpattern_set.contains(to)) {
            return false;
        }
        subpattern_set.insert(to);
        for (auto& set : banned) {
            if (set == subpattern_set) {
                return false;
            }
        }
        banned.push_back(subpattern_set);
        return true;
    }

    auto MakeCartesianProduct(PatterMatchResultImpl& graphs, PatterMatchResultImpl& edges,
                              PatterMatchResultImpl& new_result) -> void {
        std::vector<std::set<ts::ObjectId>> banned;
        for (auto& subpattern : graphs) {
            for (auto& edge : edges) {
                if (IsSetNew(subpattern.to, *edge.to.begin(), banned)) {
                    subpattern.value->AddFieldValue(edge.value->GetFields()[1]);
                    new_result.push_back(
                        {subpattern.from, subpattern.to,
                         util::Ptr<ts::Struct>(new ts::Struct(*subpattern.value))});
                    subpattern.value->RemoveLAstFieldValue();
                }
            }
        }
    }

    auto IntersectPatterMatchResults(const PatterMatchResultImpl& graphs,
                                     const PatterMatchResultImpl& edges) -> PatterMatchResultImpl {

        std::unordered_map<ts::ObjectId, PatterMatchResultImpl> from_graphs_index;
        std::unordered_map<ts::ObjectId, PatterMatchResultImpl> from_edges_index;

        for (auto& subpattern : graphs) {
            from_graphs_index[subpattern.from].push_back(subpattern);
        }
        for (auto& edge : edges) {
            from_edges_index[edge.from].push_back(edge);
        }

        PatterMatchResultImpl result;
        for (auto& [from, vector] : from_graphs_index) {
            if (from_edges_index.contains(from)) {
                MakeCartesianProduct(vector, from_edges_index[from], result);
            }
        }
        return result;
    }

    auto GenerateName(Pattern::Ptr& pattern) -> std::string {
        std::string result(pattern->GetRootClass()->Name());
        for (auto& end : pattern->GetRelations()) {
            result.append("-").append(end.relation->Name());
        }
        return result;
    }

    // Very heavy operation
    // Definitly needed review and rethinking
    auto PatternMatchImpl(Pattern::Ptr pattern) -> std::optional<PatterMatchResultImpl> {
        std::optional<PatterMatchResultImpl> result = std::nullopt;
        auto& file = alloc_->GetFile();

        auto structure_class =
            ts::NewClass<ts::StructClass>(GenerateName(pattern), pattern->GetRootClass());

        for (auto& end : pattern->GetRelations()) {
            auto pattern_result = PatternMatchImpl(end.pattern);
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
                    .ReadMagic(alloc_->GetFile())
                    .magic_;

            auto to_magic =
                mem::ClassHeader(class_storage_->FindClass(end.relation->ToClass()).value())
                    .ReadMagic(alloc_->GetFile())
                    .magic_;

            //  TODO: Manage lazy deletion
            structure_class->AddField(pattern_result.has_value()
                                          ? pattern_result.value().begin()->value->GetClass()
                                          : end.relation->ToClass());

            PatterMatchResultImpl inner_map;

            auto merge = [&from_index, &to_index, &pattern_result, &inner_map, &end, &file,
                          from_magic, to_magic, &structure_class](auto relation_node) {
                auto from = relation_node->template Data<ts::Relation>()->FromId();
                auto to = relation_node->template Data<ts::Relation>()->ToId();

                auto from_node =
                    Node(from_magic, end.relation->FromClass(), file, from_index[from]);
                auto to_node = Node(to_magic, end.relation->ToClass(), file, to_index[to]);

                if (end.predicate_(from_node, to_node)) {
                    if (pattern_result.has_value()) {
                        for (auto& subpattern : pattern_result.value()) {
                            if (subpattern.from == to) {
                                // would match cycles
                                auto new_struct = util::MakePtr<ts::Struct>(structure_class);
                                new_struct->AddFieldValue(from_node.Data<ts::Object>());
                                new_struct->AddFieldValue(subpattern.value);
                                inner_map.push_back({from, {to}, new_struct});
                            }
                        }
                    } else {
                        auto new_struct = util::MakePtr<ts::Struct>(structure_class);
                        new_struct->AddFieldValue(from_node.Data<ts::Object>());
                        new_struct->AddFieldValue(to_node.Data<ts::Object>());
                        inner_map.push_back({from, {to}, new_struct});
                    }
                }
            };

            VisitNodes(end.relation, kAll, merge);
            if (result.has_value()) {
                result = IntersectPatterMatchResults(result.value(), inner_map);
            } else {
                result = inner_map;
            }
        }

        return result;
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
    auto AddClass(const util::Ptr<C>& new_class) -> void {
        class_storage_->AddClass(new_class);
    }

    template <ts::ClassLike C>
    auto RemoveClass(const util::Ptr<C>& node_class) -> void {
        NodeStorage(node_class, class_storage_, alloc_, LOGGER).Drop();
        class_storage_->RemoveClass(node_class);
    }

    template <ts::ClassLike C>
    auto Contains(const util::Ptr<C>& node_class) -> bool {
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

    auto PrintClasses(std::ostream& os = std::cout) -> void {
        auto& alloc = alloc_;
        class_storage_->VisitClasses([alloc, &os](mem::ClassHeader class_header) -> void {
            ts::ClassObject class_object;
            class_object.Read(alloc->GetFile(),
                              mem::GetOffset(class_header.index_, class_header.free_offset_));
            os << " [ " << class_header.index_ << " ] " << class_object.ToString() << std::endl;
        });
    }

    template <ts::ObjectLike O>
    requires(!std::is_same_v<O, ts::ClassObject>) auto AddNode(util::Ptr<O> node) -> void {
        if (node->GetClass()->Size().has_value()) {
            ValNodeStorage(node->GetClass(), class_storage_, alloc_, LOGGER).AddNode(node);
        } else {
            VarNodeStorage(node->GetClass(), class_storage_, alloc_, LOGGER).AddNode(node);
        }
    }

    template <ts::ClassLike C, typename Predicate>
    auto RemoveNodesIf(const util::Ptr<C>& node_class, Predicate predicate) -> void {
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
    auto VisitNodes(const util::Ptr<C>& node_class, Predicate predicate, Functor functor) -> void {
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
    auto PrintNodesIf(util::Ptr<C>& node_class, Predicate predicate, std::ostream& os = std::cout)
        -> void {
        auto print = [&os](auto node) { os << node->ToString() << std::endl; };
        VisitNodes(node_class, predicate, print);
    }

    template <ts::ObjectLike O, typename Container, ts::ClassLike C, typename Predicate>
    auto CollectNodesIf(util::Ptr<C>& node_class,
                        std::back_insert_iterator<Container> back_inserter, Predicate predicate)
        -> void {

        auto insert = [&back_inserter](auto node) {
            *back_inserter++ = (node->template Data<O>());
        };
        VisitNodes(node_class, predicate, insert);
    }

    // Very heavy operation
    // Definitly needed review and rethinking
    template <typename Container>
    auto PatternMatch(Pattern::Ptr pattern, std::back_insert_iterator<Container> back_inserter)
        -> void {
        auto result_impl = PatternMatchImpl(pattern);
        if (result_impl.has_value()) {
            std::transform(result_impl.value().begin(), result_impl.value().end(), back_inserter,
                           [](auto& it) { return it.value; });
        }
    }
};

}  // namespace db
