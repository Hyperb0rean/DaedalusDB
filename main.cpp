#include <iostream>

#include "database.hpp"

int main() {
    auto file =
        std::make_shared<mem::File>("database.data", std::make_shared<util::ConsoleLogger>());

    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    auto coordinates_class = ts::NewClass<ts::StructClass>(
        "coordinates", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
        ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto city_class = ts::NewClass<ts::StructClass>("city", ts::NewClass<ts::StringClass>("name"),
                                                    coordinates_class);

    auto database =
        db::Database(file, db::OpenMode::kDefault, std::make_shared<util::DebugLogger>());
    // database.AddClass(person_class);
    // database.AddClass(coordinates_class);
    // database.AddClass(city_class);
    return 0;
}
