#pragma once

#include <unordered_map>

#include "types.h"

namespace db {

class Database {

    std::unique_ptr<mem::File> file_;
    std::unordered_map<std::string, std::unique_ptr<Type>> types_;

public:
    Database(std::unique_ptr<mem::File> file) : file_(std::move(file)) {
    }
    void InitializeTypes(const std::unordered_map<std::string, std::unique_ptr<Type>>& types) {
    }
};

}  // namespace db
