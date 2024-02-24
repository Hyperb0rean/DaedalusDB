
#include <ostream>

#include "test.hpp"

TEST(Relation, RelationClass) {
    auto file = util::MakePtr<mem::File>("test.data");

    auto name = ts::NewClass<ts::StringClass>("name");
    auto age = ts::NewClass<ts::PrimitiveClass<int>>("age");

    auto name_to_age = util::MakePtr<ts::RelationClass>("name-to-age", name, age);

    auto holder = util::MakePtr<ts::ClassObject>(name_to_age);
    std::cerr << holder->ToString() << std::endl;
    holder->Write(file, 0);
    holder->Read(file, 0);
    std::cerr << holder->ToString() << std::endl;
}