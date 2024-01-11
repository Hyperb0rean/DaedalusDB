#include "database.h"

namespace db {

Database::Database(std::shared_ptr<io::File>&& file) {
}

Database::Database(const std::shared_ptr<io::File>& file) {
}

template <typename T>
void Database::AddType(std::string label) {
}

template <typename T>
void Database::DeleteType(std::string label) {
}
}  // namespace db