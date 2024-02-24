#include "test.hpp"

TEST(ValNodeStorage, NodeAddition) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(coords);

    database.AddNode(ts::New<ts::Struct>(coords, 1., 0.));
    database.AddNode(ts::New<ts::Struct>(coords, 0., 1.));
}

TEST(ValNodeStorage, NodeAddditionWithAllocation) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, CONSOLE_LOGGER);
    database.AddClass(coords);

    for (size_t i = 0; i < 200; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, 1., 0.));
        database.AddNode(ts::New<ts::Struct>(coords, 0., 1.));
    }
}

TEST(ValNodeStorage, PrintNodes) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, CONSOLE_LOGGER);
    database.AddClass(coords);

    for (size_t i = 0; i < 10; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, 13., 46.));
        database.AddNode(ts::New<ts::Struct>(coords, 60., 15.));
    }

    database.PrintNodesIf(coords, [](auto it) { return it.Id() % 2 == 0; });
}

TEST(ValNodeStorage, RemoveNodes) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(coords);

    for (size_t i = 0; i < 10; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, 13., 46.));
        database.AddNode(ts::New<ts::Struct>(coords, 60., 15.));
    }

    database.RemoveNodesIf(coords, [](auto it) { return it.Id() % 2 == 0; });
    database.PrintNodesIf(coords, []() { return true; });
}

TEST(ValNodeStorage, RemoveThenAddNodes) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(coords);

    for (size_t i = 0; i < 10; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, 13., 46.));
        database.AddNode(ts::New<ts::Struct>(coords, 60., 15.));
    }

    database.RemoveNodesIf(coords, [](auto it) { return it.Id() % 2 == 0; });

    for (size_t i = 0; i < 5; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, i * 1., -1. * i));
    }

    database.PrintNodesIf(coords, []() { return true; });
}

TEST(ValNodeStorage, FreeUnusedDataPages) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(coords);

    for (size_t i = 0; i < 1000; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, 13., 46.));
    }

    database.RemoveNodesIf(coords, []() { return true; });
    database.PrintNodesIf(coords, []() { return true; });
}

TEST(ValNodeStorage, Select) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, CONSOLE_LOGGER);
    database.AddClass(coords);

    for (size_t i = 0; i < 100; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, i * 10., 1000. - i));
    }

    database.PrintNodesIf(coords, [](db::ValNodeIterator it) {
        return it->Data<ts::Struct>()->GetField<ts::Primitive<double>>("lat")->Value() >
               it->Data<ts::Struct>()->GetField<ts::Primitive<double>>("lon")->Value();
    });
}