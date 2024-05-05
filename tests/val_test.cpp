#include <__ranges/repeat_view.h>

#include <ranges>
#include <tuple>

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

    for (auto [one, zero] : std::views::repeat(std::tuple(1., 0.), 200)) {
        database.AddNode(ts::New<ts::Struct>(coords, one, zero));
        database.AddNode(ts::New<ts::Struct>(coords, zero, one));
    }
}

TEST(ValNodeStorage, PrintNodes) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, CONSOLE_LOGGER);
    database.AddClass(coords);

    for (auto [one, ten] : std::views::repeat(std::tuple(1., 10.), 10)) {
        database.AddNode(ts::New<ts::Struct>(coords, one, ten));
        database.AddNode(ts::New<ts::Struct>(coords, ten, one));
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

    for (auto [one, ten] : std::views::repeat(std::tuple(1., 10.), 10)) {
        database.AddNode(ts::New<ts::Struct>(coords, one, ten));
        database.AddNode(ts::New<ts::Struct>(coords, ten, one));
    }

    database.RemoveNodesIf(coords, [](auto it) { return it.Id() % 2 == 0; });
    database.PrintNodesIf(coords, db::kAll);
}

TEST(ValNodeStorage, RemoveThenAddNodes) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(coords);

    for (auto [one, ten] : std::views::repeat(std::tuple(1., 10.), 10)) {
        database.AddNode(ts::New<ts::Struct>(coords, one, ten));
        database.AddNode(ts::New<ts::Struct>(coords, ten, one));
    }

    database.RemoveNodesIf(coords, [](auto it) { return it.Id() % 2 == 0; });

    for (auto i : std::views::iota(0ul, 15ul)) {
        database.AddNode(ts::New<ts::Struct>(coords, i * 1., -1. * i));
    }

    database.PrintNodesIf(coords, db::kAll);
}

TEST(ValNodeStorage, FreeUnusedDataPages) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(coords);

    for ([[maybe_unused]] auto i : std::views::iota(0, 1000)) {
        database.AddNode(ts::New<ts::Struct>(coords, 13., 46.));
    }

    database.RemoveNodesIf(coords, db::kAll);
    database.PrintNodesIf(coords, db::kAll);
}

TEST(ValNodeStorage, Select) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, CONSOLE_LOGGER);
    database.AddClass(coords);

    for (auto i : std::views::iota(0, 100)) {
        database.AddNode(ts::New<ts::Struct>(coords, i * 10., 1000. - i));
    }

    database.PrintNodesIf(coords, [](db::ValNodeIterator it) {
        return it->Data<ts::Struct>()->GetField<ts::Primitive<double>>("lat")->Value() >
               it->Data<ts::Struct>()->GetField<ts::Primitive<double>>("lon")->Value();
    });
}

TEST(ValNodeStorage, IdLogic) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, CONSOLE_LOGGER);
    database.AddClass(coords);

    for ([[maybe_unused]] auto i : std::views::iota(0, 1000)) {
        database.AddNode(ts::New<ts::Struct>(coords, 10., 1000.));
    }

    database.RemoveNodesIf(coords, [](db::ValNodeIterator it) { return it->Id() < 50; });
    database.RemoveNodesIf(coords, [](db::ValNodeIterator it) { return it->Id() >= 150; });

    for ([[maybe_unused]] auto i : std::views::iota(0, 100)) {
        database.AddNode(ts::New<ts::Struct>(coords, 50., 2000.));
    }

    database.PrintNodesIf(coords, [](db::ValNodeIterator it) {
        std::cerr << it.GetRealOffset() << std::endl;
        return true;
    });
}