#include <cstddef>

#include "test.hpp"

TEST(Performance, Insert) {
    auto file = util::MakePtr<mem::File>("perf.ddb");
    auto database = db::Database(file, db::OpenMode::kWrite);

    auto name = ts::NewClass<ts::StringClass>("name");
    auto age = ts::NewClass<ts::PrimitiveClass<int>>("age");

    database.AddClass(name);
    database.AddClass(age);

    for (size_t i = 0; i < 10'000; ++i) {
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

    size_t size = 10'000;
    for (size_t i = 0; i < size; ++i) {
        database.AddNode(ts::New<ts::Primitive<int>>(age, 100000));
        database.AddNode(ts::New<ts::String>(name, "test name"));
    }

    for (size_t i = 0; i < size; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        database.RemoveNodesIf(age, [i](auto it) { return it->Id() == ID(i); });
        auto stop = std::chrono::high_resolution_clock::now();
        std::cerr << size - i << ","
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

    size_t size = 10'000;
    for (size_t i = 0; i < size; ++i) {
        database.AddNode(ts::New<ts::Primitive<int>>(age, 100000));
        database.AddNode(ts::New<ts::String>(name, "test name"));
    }

    for (size_t i = 0; i < size; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        database.RemoveNodesIf(name, [i](auto it) { return it->Id() == ID(i); });
        auto stop = std::chrono::high_resolution_clock::now();
        std::cerr << size - i << ","
                  << duration_cast<std::chrono::microseconds>(stop - start).count() << "\n";
    }
}