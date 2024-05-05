#include "test.hpp"

TEST(VarNodeStorage, NodeAddition) {
    auto name = ts::NewClass<ts::StringClass>("name");

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(name);

    database.AddNode(ts::New<ts::String>(name, "Greg"));
    database.AddNode(ts::New<ts::String>(name, "Gregory"));
    database.AddNode(ts::New<ts::String>(name, "Hyperb0rean"));
    database.PrintNodesIf(name, db::kAll);
}

TEST(VarNodeStorage, NodeAdditionWithAllocation) {
    auto name = ts::NewClass<ts::StringClass>("name");

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(name);

    for ([[maybe_unused]] auto i : std::views::iota(0, 150)) {
        database.AddNode(ts::New<ts::String>(name, "Greg"));
        database.AddNode(ts::New<ts::String>(name, "Gregory"));
        database.AddNode(ts::New<ts::String>(name, "Hyperb0rean"));
    }

    database.PrintNodesIf(name, db::kAll);
}

TEST(VarNodeStorage, NodeDeletion) {
    auto name = ts::NewClass<ts::StringClass>("name");

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(name);

    for ([[maybe_unused]] auto i : std::views::iota(0, 50)) {
        database.AddNode(ts::New<ts::String>(name, "Greg"));
        database.AddNode(ts::New<ts::String>(name, "Gregory"));
        database.AddNode(ts::New<ts::String>(name, "Hyperb0rean"));
        database.RemoveNodesIf(
            name, [](db::VarNodeIterator it) { return it->Data<ts::String>()->Value()[0] == 'G'; });
    }

    database.PrintNodesIf(name, db::kAll);
}

TEST(VarNodeStorage, NodeDeletionWithDeallocation) {
    auto name = ts::NewClass<ts::StringClass>("name");

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(name);

    for ([[maybe_unused]] auto i : std::views::iota(0, 150)) {
        database.AddNode(ts::New<ts::String>(name, "Greg"));
        database.AddNode(ts::New<ts::String>(name, "Gregory"));
        if (i > 100) {
            database.AddNode(ts::New<ts::String>(name, "Hyperb0rean"));
        }
    }
    database.RemoveNodesIf(
        name, [](db::VarNodeIterator it) { return it->Data<ts::String>()->Value()[0] == 'G'; });
    database.PrintNodesIf(name, db::kAll);
}