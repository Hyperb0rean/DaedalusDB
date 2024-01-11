#pragma once

#include <unordered_map>
#include <vector>

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

    std::unordered_map<std::string, AbstractType> types_;
    std::shared_ptr<mem::File> file_;

public:
    Database(std::shared_ptr<mem::File> file);

    template <typename T>
    [[nodiscard]] std::unique_ptr<Type<T>> AddType(std::string label);

    template <typename T>
    void DeleteType(std::string label);
};

}  // namespace db
