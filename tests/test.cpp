#include <gtest/gtest.h>

#include <iostream>

#include "database.h"

TEST(TypeTests, SimpleReadWrite) {
    auto file = std::make_shared<mem::File>("test.data");

    auto node = types::Struct("person");
    node.AddFieldValue(types::String("name", "Greg"));
    node.AddFieldValue(types::String("surname", "Sosnovtsev"));
    node.AddFieldValue(types::Primitive<int>("age", 19));

    node.Write(file, 0);
    file->Write("Cool", 4, 0, 4);
    file->Write(20, 22);

    ASSERT_EQ("person: { name: \"Greg\", surname: \"Sosnovtsev\", age: 19 }", node.ToString());
    node.Read(file, 0);
    ASSERT_EQ("person: { name: \"Cool\", surname: \"Sosnovtsev\", age: 20 }", node.ToString());
}

TEST(TypeTests, TypeDump) {
    auto file = std::make_shared<mem::File>("test.data");

    auto person_class = std::make_shared<types::StructClass>("person");
    person_class->AddField(types::StringClass("name"));
    person_class->AddField(types::StringClass("surname"));
    person_class->AddField(types::PrimitiveClass<int>("age"));

    types::ClassObject(person_class).Write(file, 1488);
    ASSERT_EQ(types::ClassObject(person_class).ToString(),
              "class: _struct@person_<_string@name__string@surname__int@age_>");

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