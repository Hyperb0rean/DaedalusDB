#include <cstddef>
#include <format>
#include <list>
#include <string>

#include "class.hpp"
#include "object.hpp"
#include "test.hpp"

TEST(Database, Collect) {
    auto database = util::MakePtr<db::Database>(util::MakePtr<mem::File>("test.data"),
                                                db::OpenMode::kWrite, CONSOLE_LOGGER);

    auto address_class = ts::NewClass<ts::StructClass>(
        "address", ts::NewClass<ts::StringClass>("city"), ts::NewClass<ts::StringClass>("street"),
        ts::NewClass<ts::PrimitiveClass<size_t>>("house"));

    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"), address_class);

    database->AddClass(person_class);
    database->PrintAllClasses();
    for (size_t i = 0; i < 100; ++i) {
        database->AddNode(ts::New<ts::Struct>(person_class, std::format("Greg {}", i), "Sosnovtsev",
                                              19, "Saint-Petersburg", "Lomonosova", i));
    }

    auto list = database->CollectNodesIf<ts::Struct, std::list<ts::Struct::Ptr>>(
        person_class, [](db::VarNodeIterator it) {
            return it->Data<ts::Struct>()
                       ->GetField<ts::Struct>("address")
                       ->GetField<ts::Primitive<size_t>>("house")
                       ->Value() > 90;
        });

    for (auto& ptr : list) {
        std::cout << ptr->ToString() << std::endl;
    }
}