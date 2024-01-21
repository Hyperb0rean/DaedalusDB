#include <gtest/gtest.h>

#include <iostream>
#include <string>

#include "database.hpp"

using namespace std::string_literals;

TEST(TypeTests, SimpleReadWrite) {
    auto file = std::make_shared<mem::File>("test.data");
    auto name = ts::NewClass<ts::StringClass>("name");
    auto node = ts::New<ts::String>(name, "Greg");
    node->Write(file, 0);
    file->Write("Cool", 4, 0, 4);

    ASSERT_EQ("name: \"Greg\"", node->ToString());
    node->Read(file, 0);
    ASSERT_EQ("name: \"Cool\"", node->ToString());
}

TEST(TypeTests, ReadWrite) {
    auto file = std::make_shared<mem::File>("test.data");

    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    auto node = ts::New<ts::Struct>(person_class, "Greg"s, "Sosnovtsev"s, 19, true);

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

void PreInitPages(size_t number, std::shared_ptr<mem::File>& file) {
    auto page = mem::Page();
    page.next_page_index_ = 1;
    page.previous_page_index_ = mem::kDummyIndex;
    file->Write<mem::Page>(page, 60);
    for (size_t i = 1; i < number - 1; ++i) {
        auto page = mem::Page(i);
        page.previous_page_index_ = i - 1;
        page.next_page_index_ = i + 1;
        file->Write<mem::Page>(page, 60 + mem::kPageSize * i);
    }
    page = mem::Page(number - 1);
    page.next_page_index_ = mem::kDummyIndex;
    page.previous_page_index_ = number - 2;

    file->Write<mem::Page>(page, 60 + mem::kPageSize * (number - 1));

    page = mem::Page(mem::kDummyIndex);
    page.previous_page_index_ = number - 1;
    page.next_page_index_ = 0;
    file->Write<mem::Page>(page);
    file->Write<size_t>(number, mem::kFreePagesCountOffset);
}

TEST(PageList, SimpleIteration) {
    auto file = std::make_shared<mem::File>("test.data");

    PreInitPages(1000, file);

    auto alloc = std::make_shared<mem::PageAllocator>(file, 60);
    size_t index = 999;
    for (auto& page : mem::PageList(alloc, 0)) {
        ASSERT_EQ(page.index_, index--);
    }
}

TEST(PageList, Unlink) {
    auto file = std::make_shared<mem::File>("test.data");

    PreInitPages(6, file);

    auto alloc = std::make_shared<mem::PageAllocator>(file, 60);
    size_t index = 4;
    auto list = mem::PageList(alloc, 0);
    list.Unlink(3);
    list.Unlink(5);
    for (auto& page : list) {
        std::cerr << page.index_ << ' ';
        if (index == 3) {
            index--;
        }
        ASSERT_EQ(page.index_, index--);
    }
}

TEST(PageList, LinkBefore) {
    auto file = std::make_shared<mem::File>("test.data");

    PreInitPages(10, file);

    auto alloc = std::make_shared<mem::PageAllocator>(file, 60);
    auto list = mem::PageList(alloc, 0);
    list.Unlink(3);
    list.Unlink(5);
    list.LinkBefore(6, 3);

    size_t index = 9;
    for (auto& page : list) {
        std::cerr << page.index_ << ' ';
        if (index == 3) {
            index--;
            ASSERT_EQ(page.index_, index--);
        } else if (index == 5) {
            ASSERT_EQ(page.index_, 3);
            index--;
        } else {
            ASSERT_EQ(page.index_, index--);
        }
    }
}

TEST(PageList, Push) {
    auto file = std::make_shared<mem::File>("test1.data");
    file->Write<mem::Page>(mem::Page(mem::kDummyIndex));
    file->Write<size_t>(0, mem::kFreePagesCountOffset);
    file->Write<size_t>(0, mem::kPagesCountOffset);
    std::cerr << file->GetSize() << '\n';

    auto alloc = std::make_shared<mem::PageAllocator>(file, 72);
    auto list = mem::PageList(alloc, 0);

    for (size_t i = 0; i < 1; ++i) {
        list.PushBack(alloc->AllocatePage());
        if (i % 2 == 0) {
            list.Unlink(i);
        }
    }

    size_t index = 1;
    for (auto& page : list) {
        ASSERT_EQ(page.index_, index);
        index += 2;
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}