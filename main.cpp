#include <iostream>

#include "db_struct/database.h"

int main() {
    auto file = std::make_shared<mem::File>("database.data");

    auto database = std::make_unique<db::Database>(file);

    struct Point {
        int x, y;
    };
    auto point = database->AddType<Point>("point");
    file->Write<int>({4, 5, 6});
    auto five = file->Read<int>(4);
    std::cout << five;
    return 0;
}