#include <iostream>

#include "db_struct/database.h"

int main() {
    auto file = std::make_shared<io::File>("database.data");

    auto database = std::make_unique<db::Database>(file);

    struct Point {
        int x, y;
    };
    auto point = database->AddType<Point>("point");

    return 0;
}