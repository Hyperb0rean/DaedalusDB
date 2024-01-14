#pragma once

#include <unordered_map>

#include "types.h"

namespace db {

class Database {

    std::shared_ptr<mem::File> file_;
    std::unordered_map<std::string, Type> types_;

public:
    Database(std::shared_ptr<mem::File> file) : file_(file) {
    }
    void InitializeTypes(const std::unordered_map<std::string, Type>& types) {
    }
};

}  // namespace db
