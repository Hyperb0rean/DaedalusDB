#include <gtest/gtest.h>

#include <iostream>
#include <string>

#include "database.hpp"

using namespace std::string_literals;

TEST(TypeTests, SimpleReadWrite) {
    auto file = std::make_shared<mem::File>("test.data");
    file->Clear();
    auto name = ts::NewClass<ts::StringClass>("name");
    auto node = ts::New<ts::String>(name, "Greg");
    node->Write(file, 0);
    file->Write("Cool", 4, 0, 4);

    ASSERT_EQ("name: \"Greg\"", node->ToString());
    node->Read(file, 0);
    ASSERT_EQ("name: \"Cool\"", node->ToString());
}

TEST(TypeTests, InvalidClasses) {
    ASSERT_THROW(auto name = ts::NewClass<ts::StringClass>("name_"), error::TypeError);
    ASSERT_THROW(auto name = ts::NewClass<ts::StringClass>("n@me"), error::TypeError);
    ASSERT_THROW(auto name = ts::NewClass<ts::StringClass>("<name>"), error::TypeError);
}

TEST(TypeTests, ReadWrite) {
    auto file = std::make_shared<mem::File>("test.data");
    file->Clear();
    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    auto node = ts::New<ts::Struct>(person_class, "Greg"s, "Sosnovtsev"s, 19, true);

    node->Write(file, 0);
    file->Write("Cool", 4, 0, 4);
    file->Write(20, 22);

    ASSERT_EQ("person: { name: \"Greg\", surname: \"Sosnovtsev\", age: 19, male: true }",
              node->ToString());
    node->Read(file, 0);
    ASSERT_EQ("person: { name: \"Cool\", surname: \"Sosnovtsev\", age: 20, male: true }",
              node->ToString());
}

TEST(TypeTests, SafeNew) {
    auto file = std::make_shared<mem::File>("test.data");
    file->Clear();
    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    ASSERT_THROW(auto node = ts::New<ts::Struct>(person_class, "Greg"s, "Sosnovtsev"s),
                 error::BadArgument);
}

TEST(TypeTests, DefaultNew) {
    auto file = std::make_shared<mem::File>("test.data");
    file->Clear();
    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    auto node = ts::DefaultNew<ts::Struct>(person_class);

    ASSERT_EQ("person: { name: \"\", surname: \"\", age: 0, male: false }", node->ToString());
}

TEST(TypeTests, ReadNew) {
    auto file = std::make_shared<mem::File>("test.data");
    file->Clear();
    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    auto node = ts::New<ts::Struct>(person_class, "Greg"s, "Sosnovtsev"s, 19, true);
    node->Write(file, 0);
    ASSERT_EQ("person: { name: \"Greg\", surname: \"Sosnovtsev\", age: 19, male: true }",
              node->ToString());
    auto new_node = ReadNew<ts::Struct>(person_class, file, 0);
    ASSERT_EQ(node->ToString(), new_node->ToString());
}

TEST(TypeTests, TypeDump) {
    auto file = std::make_shared<mem::File>("test.data");
    file->Clear();
    auto person_class = std::make_shared<ts::StructClass>("person");
    person_class->AddField(ts::StringClass("name"));
    person_class->AddField(ts::StringClass("surname"));
    person_class->AddField(ts::PrimitiveClass<int>("age"));
    person_class->AddField(ts::PrimitiveClass<uint64_t>("money"));

    ts::ClassObject(person_class).Write(file, 1488);
    ASSERT_EQ(ts::ClassObject(person_class).ToString(),
              "_struct@person_<_string@name__string@surname__int@age__longunsignedint@money_>");

    ts::ClassObject read_class;
    read_class.Read(file, 1488);
    ASSERT_EQ(read_class.ToString(), ts::ClassObject(person_class).ToString());
}

TEST(TypeTests, Metadata) {
    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    ASSERT_TRUE(ts::ClassObject(person_class).Contains(ts::NewClass<ts::StringClass>("surname")));
    ASSERT_FALSE(
        ts::ClassObject(person_class).Contains(ts::NewClass<ts::PrimitiveClass<int>>("surname")));
    ASSERT_FALSE(ts::ClassObject(person_class).Contains(ts::NewClass<ts::StringClass>("address")));
}

TEST(ClassStorage, ClassAddition) {
    auto file = std::make_shared<mem::File>("test.data", std::make_shared<util::ConsoleLogger>());
    file->Clear();
    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    auto coordinates_class = ts::NewClass<ts::StructClass>(
        "coordinates", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
        ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto city_class = ts::NewClass<ts::StructClass>("city", ts::NewClass<ts::StringClass>("name"),
                                                    coordinates_class);

    auto database =
        db::Database(file, db::OpenMode::kWrite, std::make_shared<util::ConsoleLogger>());
    database.AddClass(person_class);
    database.AddClass(coordinates_class);
    database.AddClass(city_class);
}

TEST(ClassStorage, PrintClasses) {
    auto database = db::Database(std::make_shared<mem::File>("test.data"));
    database.PrintClasses();
}

TEST(ClassStorage, ClassRemoval) {
    auto file = std::make_shared<mem::File>("test.data", std::make_shared<util::ConsoleLogger>());

    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    auto coordinates_class = ts::NewClass<ts::StructClass>(
        "coordinates", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
        ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto city_class = ts::NewClass<ts::StructClass>("city", ts::NewClass<ts::StringClass>("name"),
                                                    coordinates_class);

    auto database =
        db::Database(file, db::OpenMode::kRead, std::make_shared<util::ConsoleLogger>());
    database.RemoveClass(person_class);
    database.RemoveClass(coordinates_class);
    // database.RemoveClass(city_class);
    database.PrintClasses();
}

TEST(NodeStorage, NodeAddition) {
    auto file = std::make_shared<mem::File>("test.data", std::make_shared<util::ConsoleLogger>());
    file->Clear();
    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    auto database =
        db::Database(file, db::OpenMode::kWrite, std::make_shared<util::ConsoleLogger>());
    database.AddClass(person_class);

    database.AddNode(ts::New<ts::Struct>(person_class, "Greg", "Sosnovtsev", 19, true));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}