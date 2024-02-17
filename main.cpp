#include <iostream>

#include "database.hpp"

int main() {

    auto coordinates_class = ts::NewClass<ts::StructClass>(
        "coordinates", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
        ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database = db::Database(std::make_shared<mem::File>("database.data"),
                                 db::OpenMode::kDefault, CONSOLE_LOGGER);
    database.AddClass(coordinates_class);

    database.AddNode(ts::New<ts::Struct>(coordinates_class, 31., 55.));
    database.AddNode(ts::DefaultNew<ts::Struct>(coordinates_class));

    database.PrintAllClasses();
    database.PrintAllNodes(coordinates_class);

    database.RemoveNodesIf(coordinates_class, [](db::ValNodeIterator it) { return it.Id() != 0; });
    database.AddNode(ts::New<ts::Struct>(coordinates_class, 31., 55.));

    database.PrintAllNodes(coordinates_class);
    database.RemoveClass(coordinates_class);

    return 0;
}
