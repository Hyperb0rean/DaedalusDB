#include <gtest/gtest.h>

#include <iostream>
#include <string>

#include "database.hpp"

TEST(TypeTests, NewTest) {
    auto file = std::make_shared<mem::File>("test.data");
    auto name = ts::NewClass<ts::StringClass>("name");
    auto node = ts::New<ts::String>(name, "Greg");
    node->Write(file, 0);
    file->Write("Cool", 4, 0, 4);

    ASSERT_EQ("name: \"Greg\"", node->ToString());
    node->Read(file, 0);
    ASSERT_EQ("name: \"Cool\"", node->ToString());
}

TEST(TypeTests, SimpleReadWrite) {
    auto file = std::make_shared<mem::File>("test.data");

    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    auto node =
        ts::New<ts::Struct>(person_class, std::string{"Greg"}, std::string{"Sosnovtsev"}, 19, true);

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

    auto person_class = std::make_shared<ts::StructClass>("person");
    person_class->AddField(ts::StringClass("name"));
    person_class->AddField(ts::StringClass("surname"));
    person_class->AddField(ts::PrimitiveClass<int>("age"));
    person_class->AddField(ts::PrimitiveClass<uint64_t>("money"));

    ts::ClassObject(person_class).Write(file, 1488);
    ASSERT_EQ(
        ts::ClassObject(person_class).ToString(),
        "class: _struct@person_<_string@name__string@surname__int@age__longunsignedint@money_>");

    ts::ClassObject read_class;
    read_class.Read(file, 1488);
    ASSERT_EQ(read_class.ToString(), ts::ClassObject(person_class).ToString());
}

TEST(TypeTests, SyntaxSugarClasses) {
    auto file = std::make_shared<mem::File>("test.data");

    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<uint64_t>>("money"));

    ts::ClassObject(person_class).Write(file, 1337);
    ASSERT_EQ(
        ts::ClassObject(person_class).ToString(),
        "class: _struct@person_<_string@name__string@surname__int@age__longunsignedint@money_>");

    ts::ClassObject read_class;
    read_class.Read(file, 1488);
    ASSERT_EQ(read_class.ToString(), ts::ClassObject(person_class).ToString());
}

void PreInitPages(size_t number, std::shared_ptr<mem::File>& file) {
    auto page = mem::Page();
    page.next_page_index_ = 1;
    page.previous_page_index_ = number;
    file->Write<mem::Page>(page, 60);
    for (size_t i = 1; i < number - 1; ++i) {
        auto page = mem::Page(i);
        page.previous_page_index_ = i - 1;
        page.next_page_index_ = i + 1;
        file->Write<mem::Page>(page, 60 + mem::kPageSize * i);
    }
    page = mem::Page(number - 1);
    page.next_page_index_ = number;
    page.previous_page_index_ = number - 2;

    file->Write<mem::Page>(page, 60 + mem::kPageSize * (number - 1));

    page = mem::Page(number);
    page.previous_page_index_ = number - 1;
    page.next_page_index_ = 0;
    file->Write<mem::Page>(page);
}

TEST(PageIterator, SimpleIteration) {
    auto file = std::make_shared<mem::File>("test.data");

    PreInitPages(1000, file);

    auto alloc = std::make_shared<mem::PageAllocator>(file, 60, 1000, 0);
    size_t index = 0;
    for (auto& page : mem::PageList(alloc, 0)) {
        ASSERT_EQ(page.index_, index++);
    }
}

TEST(PageIterator, Unlink) {
    auto file = std::make_shared<mem::File>("test.data");

    PreInitPages(6, file);

    auto alloc = std::make_shared<mem::PageAllocator>(file, 60, 6, 0);
    size_t index = 0;
    auto list = mem::PageList(alloc, 0);
    list.Unlink(3);
    list.Unlink(5);
    for (auto& page : list) {
        std::cerr << page.index_ << ' ';
        if (index == 3) {
            index++;
        }
        ASSERT_EQ(page.index_, index++);
    }
}

TEST(PageIterator, LinkBefore) {
    auto file = std::make_shared<mem::File>("test.data");

    PreInitPages(10, file);

    auto alloc = std::make_shared<mem::PageAllocator>(file, 60, 10, 0);
    auto list = mem::PageList(alloc, 0);
    list.Unlink(3);
    list.Unlink(5);
    list.LinkBefore(6, 3);

    size_t index = 0;
    for (auto& page : list) {
        std::cerr << page.index_ << ' ';
        if (index == 3) {
            index++;
            ASSERT_EQ(page.index_, index++);
        } else if (index == 5) {
            ASSERT_EQ(page.index_, 3);
            index++;
        } else {
            ASSERT_EQ(page.index_, index++);
        }
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}