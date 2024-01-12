#pragma once

#include <unordered_map>

#include "../mem/mem.h"

namespace db {

class AbstractType {
public:
    virtual ~AbstractType() = 0;
};

template <typename T>
class Type : AbstractType {
    std::vector<T> instances_;

public:
    ~Type() = default;
    void AddNode(T data);
    void DeleteNode();
};

class Database {

    std::shared_ptr<mem::File> file_;

public:
    Database(std::shared_ptr<mem::File> file) : file_(file) {
    }

    template <typename T>
    [[nodiscard]] std::unique_ptr<Type<T>> AddType(std::string label) {
        return nullptr;
    }

    template <typename T>
    void DeleteType(std::string label) {
    }
};

}  // namespace db
