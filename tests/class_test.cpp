#include "test.hpp"

TEST(ClassStorage, ClassAddition) {
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
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, CONSOLE_LOGGER);
    database.AddClass(person_class);
    database.AddClass(coordinates_class);
    database.AddClass(city_class);
}

TEST(ClassStorage, PrintClasses) {
    auto database = db::Database(util::MakePtr<mem::File>("test.data"));
    database.PrintClasses();
}

TEST(ClassStorage, ClassRemoval) {
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
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kRead, CONSOLE_LOGGER);
    database.RemoveClass(person_class);
    database.RemoveClass(coordinates_class);
    // database.RemoveClass(city_class);
    database.PrintClasses();
}