#pragma once

#include <functional>

#include "node.hpp"

namespace db {

constexpr auto kAll = [](auto) { return true; };

// Currently will support only DAG-like structures ?

class PatternBase {
public:
    using Ptr = util::Ptr<PatternBase>;
    PatternBase() {
    }
    virtual ~PatternBase(){};
};

class Pattern : public PatternBase {
private:
    ts::Class::Ptr root_;
    struct End {
        ts::RelationClass::Ptr relation;
        std::function<bool(Node, Node)> predicate_;
        PatternBase::Ptr pattern;
    };
    using Relations = std::vector<End>;
    Relations relations_;

public:
    using Ptr = util::Ptr<Pattern>;
    ~Pattern() = default;
    Pattern(const ts::Class::Ptr& root) : root_(root) {
    }

    void AddRelation(ts::RelationClass::Ptr relation, std::function<bool(Node, Node)> predicate,
                     Pattern::Ptr pattern) {
        if (relation->FromClass()->Serialize() == root_->Serialize()) {
            relations_.emplace_back(relation, predicate, pattern);
        }
    }

    void AddRelation(ts::RelationClass::Ptr relation, std::function<bool(Node, Node)> predicate) {
        AddRelation(relation, predicate, util::MakePtr<Pattern>(relation->ToClass()));
    }

    Relations GetRelations() {
        return relations_;
    }
};

}  // namespace db