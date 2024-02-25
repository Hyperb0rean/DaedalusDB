#pragma once

#include <functional>
#include <iterator>
#include <unordered_map>

#include "new.hpp"
#include "node.hpp"

namespace db {

constexpr auto kAll = [](auto) { return true; };

template <typename Predicate, typename Iterator>
concept BinaryPredicateVal = requires(Predicate pred, Iterator it) {
    requires std::forward_iterator<Iterator>;
    { pred(it, it) } -> std::convertible_to<bool>;
};

// Currently will support only DAG-like structures ?

class PatternBase {
public:
    using Ptr = util::Ptr<PatternBase>;
    PatternBase() {
    }
    virtual ~PatternBase(){};
};

// class RealtionPredicateBase {
// public:
//     using Ptr = util::Ptr<RealtionPredicateBase>;
//     ~RealtionPredicateBase(){};
// };

// class RelationPredicate {
// public:
//     using Ptr = util::Ptr<RelationPredicate<FromIterator, ToIterator>>;
//     ~RelationPredicate() = default;
//     std::function<bool(FromIterator, ToIterator)> predicate_;

//     // intentionally implicit
//     RelationPredicate(std::function<bool(FromIterator, ToIterator)> predicate)
//         : predicate_((predicate)) {
//     }
// };

class Pattern : public PatternBase {
private:
    ts::Class::Ptr root_;
    // enum class Color { kWhite, kGray, kBlack };
    struct End {
        // Color color_;
        ts::RelationClass::Ptr relation;
        std::function<bool(Node, Node)> predicate_;
        PatternBase::Ptr pattern;
    };
    using Relations = std::unordered_map<std::string, End>;
    Relations relations_;

public:
    using Ptr = util::Ptr<Pattern>;
    ~Pattern() = default;
    Pattern(const ts::Class::Ptr& root) : root_(root) {
    }

    void AddRelation(ts::RelationClass::Ptr relation, std::function<bool(Node, Node)> predicate) {
        if (relation->FromClass()->Serialize() == root_->Serialize()) {
            auto ptr = util::MakePtr<Pattern>(relation->ToClass());
            relations_[relation->ToClass()->Serialize()] = End{relation, predicate, ptr};
        } else {
            for (auto& r : relations_) {
                try {
                    util::As<Pattern>(r.second.pattern)->AddRelation(relation, predicate);
                    return;
                } catch (const error::PatternError& err) {
                    continue;
                }
            }
            throw error::PatternError("Can't insert this relation into pattern");
        }
    }

    Relations GetRelations() {
        return relations_;
    }
};

}  // namespace db