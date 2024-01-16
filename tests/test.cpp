#include <gtest/gtest.h>

#include <iostream>

#include "database.h"

TEST(TypeTests, SimpleReadWrite) {
    auto file = std::make_shared<mem::File>("test.data");

    auto node = types::Struct("person");
    node.AddFieldValue(types::Primitive<std::string>("name", "Greg"));
    node.AddFieldValue(types::Primitive<std::string>("surname", "Sosnovtsev"));
    node.AddFieldValue(types::Primitive<int>("age", 19));

    node.Write(file, 0);
    file->Write("Cool", 4, 0, 4);
    file->Write(20, 22);

    ASSERT_EQ("person: { name: \"Greg\", surname: \"Sosnovtsev\", age: 19 }", node.ToString());
    node.Read(file, 0);
    ASSERT_EQ("person: { name: \"Cool\", surname: \"Sosnovtsev\", age: 20 }", node.ToString());
}

TEST(PageRangeTests, SimpleIteration) {
    auto file = std::make_shared<mem::File>("test.data");

    auto range = mem::PageRange(0, 5);
    for (size_t i = 0; i < 5; ++i) {
        file->Write<mem::Page>({mem::PageType::kAllocatorMetadata, i, 0, 60 + mem::kPageSize * i},
                               60 + mem::kPageSize * i);
    }

    auto alloc = mem::PageAllocator(file, 60, 0, 0, 0);

    int index = 0;
    for (auto& page : alloc.GetPageRange(0, 5)) {
        ASSERT_EQ(page.index, index++);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}