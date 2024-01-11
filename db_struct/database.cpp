#include "database.h"

namespace db {

Database::Database(std::shared_ptr<io::File> file) : file_(file) {
}

template <typename T>
std::unique_ptr<Type<T>> Database::AddType(std::string label) {
}

template <typename T>
void Database::DeleteType(std::string label) {
}
}  // namespace db