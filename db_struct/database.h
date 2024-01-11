#pragma once

#include <unordered_map>
#include <vector>

#include "../mem/io.h"

namespace db {

class AbstractHeader {
public:
    virtual ~AbstractHeader() = 0;
};

template <typename T>
class TypeHeader : AbstractHeader {
    std::vector<T> instances_;

public:
};

class Database {

    struct Superblock {
        size_t nodes;
        size_t relations;
    } superblock_;

    std::unordered_map<std::string, AbstractHeader> types_;
    std::shared_ptr<io::File> file_;

public:
    Database(std::shared_ptr<io::File>&& file);
    Database(const std::shared_ptr<io::File>& file);

    template <typename T>
    void AddType(std::string label);

    template <typename T>
    void DeleteType(std::string label);
};

}  // namespace db
