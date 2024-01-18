#include <gtest/gtest.h>

#include <iostream>
#include <string>

#include "database.h"

TEST(TypeTests, NewTest) {
    auto file = std::make_shared<mem::File>("test.data");
    auto name = types::DeclareClass<types::StringClass>("name");
    auto node = types::New<types::String>(name, "Greg");
    node->Write(file, 0);
    file->Write("Cool", 4, 0, 4);
    // file->Write(20, 22);

    ASSERT_EQ("name: \"Greg\"", node->ToString());
    node->Read(file, 0);
    ASSERT_EQ("name: \"Cool\"", node->ToString());
}

TEST(TypeTests, SimpleReadWrite) {
    auto file = std::make_shared<mem::File>("test.data");

    auto person_class = types::DeclareClass<types::StructClass>(
        "person", types::DeclareClass<types::StringClass>("name"),
        types::DeclareClass<types::StringClass>("surname"),
        types::DeclareClass<types::PrimitiveClass<int>>("age"),
        types::DeclareClass<types::PrimitiveClass<bool>>("male"));

    auto node = types::New<types::Struct>(person_class, std::string{"Greg"},
                                          std::string{"Sosnovtsev"}, 19, true);

    node->Write(file, 0);
    file->Write("Cool", 4, 0, 4);
    file->Write(20, 22);

    ASSERT_EQ("person: { name: \"Greg\", surname: \"Sosnovtsev\", age: 19, male: 1 }",
              node->ToString());
    node->Read(file, 0);
    ASSERT_EQ("person: { name: \"Cool\", surname: \"Sosnovtsev\", age: 20, male: 1 }",
              node->ToString());
}

TEST(TypeTests, TypeDump) {
    auto file = std::make_shared<mem::File>("test.data");

    auto person_class = std::make_shared<types::StructClass>("person");
    person_class->AddField(types::StringClass("name"));
    person_class->AddField(types::StringClass("surname"));
    person_class->AddField(types::PrimitiveClass<int>("age"));
    person_class->AddField(types::PrimitiveClass<uint64_t>("money"));

    types::ClassObject(person_class).Write(file, 1488);
    ASSERT_EQ(
        types::ClassObject(person_class).ToString(),
        "class: _struct@person_<_string@name__string@surname__int@age__longunsignedint@money_>");

    types::ClassObject read_class;
    read_class.Read(file, 1488);
    ASSERT_EQ(read_class.ToString(), types::ClassObject(person_class).ToString());
}

TEST(TypeTests, SyntaxSugarClasses) {
    auto file = std::make_shared<mem::File>("test.data");

    auto person_class = types::DeclareClass<types::StructClass>(
        "person", types::DeclareClass<types::StringClass>("name"),
        types::DeclareClass<types::StringClass>("surname"),
        types::DeclareClass<types::PrimitiveClass<int>>("age"),
        types::DeclareClass<types::PrimitiveClass<uint64_t>>("money"));

    types::ClassObject(person_class).Write(file, 1337);
    ASSERT_EQ(
        types::ClassObject(person_class).ToString(),
        "class: _struct@person_<_string@name__string@surname__int@age__longunsignedint@money_>");

    types::ClassObject read_class;
    read_class.Read(file, 1488);
    ASSERT_EQ(read_class.ToString(), types::ClassObject(person_class).ToString());
}

TEST(PageIterator, SimpleIteration) {
    auto file = std::make_shared<mem::File>("test.data");

    file->Write<mem::Page>({mem::PageType::kFree, 0, 0, 0, 0, 1}, 60);
    for (size_t i = 1; i < 5; ++i) {
        file->Write<mem::Page>({mem::PageType::kFree, i, 0, 0, (i - 1), (i + 1)},
                               60 + mem::kPageSize * i);
    }
    file->Write<mem::Page>({mem::PageType::kFree, 5, 0, 0, 4, 5}, 60 + mem::kPageSize * 5);

    auto alloc = std::make_shared<mem::PageAllocator>(file, 60, 6, 0);
    int index = 0;
    for (auto it = alloc->GetFreeList(); it->index < alloc->GetPagesCount(); ++it) {
        ASSERT_EQ(it->index, index++);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}