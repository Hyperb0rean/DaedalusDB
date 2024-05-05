#include <cstddef>
#include <ranges>

#include "logger.hpp"
#include "pattern.hpp"
#include "test.hpp"

TEST(Performance, Insert) {
    auto file = util::MakePtr<mem::File>("perf.ddb");
    auto database = db::Database(file, db::OpenMode::kWrite);

    auto name = ts::NewClass<ts::StringClass>("name");
    auto age = ts::NewClass<ts::PrimitiveClass<int>>("age");

    database.AddClass(name);
    database.AddClass(age);

    for (auto i : std::views::iota(0, 10'000)) {
        auto start = std::chrono::high_resolution_clock::now();
        database.AddNode(ts::New<ts::String>(name, "test name"));
        auto stop = std::chrono::high_resolution_clock::now();
        std::cerr << 2 * i + 1 << ","
                  << duration_cast<std::chrono::microseconds>(stop - start).count() << "\n";

        start = std::chrono::high_resolution_clock::now();
        database.AddNode(ts::New<ts::Primitive<int>>(age, 100));
        stop = std::chrono::high_resolution_clock::now();
        std::cerr << 2 * i + 2 << ","
                  << duration_cast<std::chrono::microseconds>(stop - start).count() << "\n";
    }
}

TEST(Performance, RemoveVal) {
    auto file = util::MakePtr<mem::File>("perf.ddb");
    auto database = db::Database(file, db::OpenMode::kWrite);

    auto name = ts::NewClass<ts::StringClass>("name");
    auto age = ts::NewClass<ts::PrimitiveClass<int>>("age");

    database.AddClass(name);
    database.AddClass(age);

    int size = 10'000;
    for ([[maybe_unused]] auto i : std::views::iota(0, size)) {
        database.AddNode(ts::New<ts::Primitive<int>>(age, 100000));
        database.AddNode(ts::New<ts::String>(name, "test name"));
    }

    for ([[maybe_unused]] auto i : std::views::iota(0, size)) {
        auto start = std::chrono::high_resolution_clock::now();
        database.RemoveNodesIf(age, [i](auto it) { return it->Id() == ID(i); });
        auto stop = std::chrono::high_resolution_clock::now();
        std::cerr << 2 * size - i << ","
                  << duration_cast<std::chrono::microseconds>(stop - start).count() << "\n";
    }
}

TEST(Performance, RemoveVar) {
    auto file = util::MakePtr<mem::File>("perf.ddb");
    auto database = db::Database(file, db::OpenMode::kWrite);

    auto name = ts::NewClass<ts::StringClass>("name");
    auto age = ts::NewClass<ts::PrimitiveClass<int>>("age");

    database.AddClass(name);
    database.AddClass(age);

    int size = 10'000;
    for ([[maybe_unused]] auto i : std::views::iota(0, size)) {
        database.AddNode(ts::New<ts::Primitive<int>>(age, 100000));
        database.AddNode(ts::New<ts::String>(name, "test name"));
    }

    for ([[maybe_unused]] auto i : std::views::iota(0, size)) {
        auto start = std::chrono::high_resolution_clock::now();
        database.RemoveNodesIf(name, [i](auto it) { return it->Id() == ID(i); });
        auto stop = std::chrono::high_resolution_clock::now();
        std::cerr << 2 * size - i << ","
                  << duration_cast<std::chrono::microseconds>(stop - start).count() << "\n";
    }
}

TEST(Performance, Compression) {
    auto file = util::MakePtr<mem::File>("perf.ddb");
    auto database = db::Database(file, db::OpenMode::kWrite);

    auto name = ts::NewClass<ts::StringClass>("name");
    auto age = ts::NewClass<ts::PrimitiveClass<int>>("age");

    database.AddClass(name);
    database.AddClass(age);

    int size = 10'000;
    for (auto i : std::views::iota(0, size)) {
        database.AddNode(ts::New<ts::String>(name, "test name"));
        std::cerr << i << "," << file->GetSize() << "\n";
    }
    for (auto i : std::views::iota(0, size) | std::views::reverse) {
        database.RemoveNodesIf(name, [i](auto it) { return it->Id() == ID(i); });
        std::cerr << i << "," << file->GetSize() << "\n";
    }
    for (auto i : std::views::iota(0, size)) {
        database.AddNode(ts::New<ts::Primitive<int>>(age, 100000));
        std::cerr << i << "," << file->GetSize() << "\n";
    }
    for (auto i : std::views::iota(0, size) | std::views::reverse) {
        database.RemoveNodesIf(age, [i](auto it) { return it->Id() == ID(i); });
        std::cerr << i << "," << file->GetSize() << "\n";
    }
}

TEST(Performance, Match) {
    auto point = ts::NewClass<ts::PrimitiveClass<int>>("point");
    auto edge = ts::NewClass<ts::RelationClass>("edge", point, point);

    auto database =
        util::MakePtr<db::Database>(util::MakePtr<mem::File>("perf.data"), db::OpenMode::kWrite);
    database->AddClass(point);
    database->AddClass(edge);

    int size = 1'00;
    int operaption = 0;
    for (auto i : std::views::iota(0, size)) {
        database->AddNode(ts::New<ts::Primitive<int>>(point, i));
    }
    for (auto i : std::views::iota(1, size)) {
        database->AddNode(ts::New<ts::Relation>(edge, ID(0), ID(i)));
        database->AddNode(ts::New<ts::Relation>(edge, ID(i), ID(0)));
    }
    for (auto i : std::views::iota(1, size)) {
        for (auto j : std::views::iota(1, i)) {
            auto star = util::MakePtr<db::Pattern>(point);
            star->AddRelation(edge, [i](db::Node a, db::Node b) {
                return a.Data<ts::Primitive<int>>()->Value() == 0 &&
                       b.Data<ts::Primitive<int>>()->Value() == i;
            });
            star->AddRelation(edge, [j](db::Node a, db::Node b) {
                return a.Data<ts::Primitive<int>>()->Value() == 0 &&
                       b.Data<ts::Primitive<int>>()->Value() == j;
            });
            std::vector<ts::Struct::Ptr> result;
            auto start = std::chrono::high_resolution_clock::now();
            database->PatternMatch(star, std::back_inserter(result));
            auto stop = std::chrono::high_resolution_clock::now();

            std::cerr << operaption++ << ","
                      << duration_cast<std::chrono::microseconds>(stop - start).count() << "\n";
        }
    }
}
