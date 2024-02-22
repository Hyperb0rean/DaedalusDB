#include <gtest/gtest.h>

#include <iostream>
#include <string>

#include "database.hpp"

using namespace std::string_literals;

TEST(TypeTests, SimpleReadWrite) {
    auto file = util::MakePtr<mem::File>("test.data");
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
    auto file = util::MakePtr<mem::File>("test.data");
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
    auto file = util::MakePtr<mem::File>("test.data");
    file->Clear();
    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    ASSERT_THROW(auto node = ts::New<ts::Struct>(person_class, "Greg"s, "Sosnovtsev"s),
                 error::BadArgument);
}

TEST(TypeTests, DefaultNew) {
    auto file = util::MakePtr<mem::File>("test.data");
    file->Clear();
    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    auto node = ts::DefaultNew<ts::Struct>(person_class);

    ASSERT_EQ("person: { name: \"\", surname: \"\", age: 0, male: false }", node->ToString());
}

TEST(TypeTests, ReadNew) {
    auto file = util::MakePtr<mem::File>("test.data");
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
    auto file = util::MakePtr<mem::File>("test.data");
    file->Clear();
    // old syntax
    // auto person_class = util::MakePtr<ts::StructClass>("person");
    // person_class->AddField(ts::StringClass("name"));
    // person_class->AddField(ts::StringClass("surname"));
    // person_class->AddField(ts::PrimitiveClass<int>("age"));
    // person_class->AddField(ts::PrimitiveClass<uint64_t>("money"));

    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<uint64_t>>("money"));

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

    ASSERT_TRUE(ts::ClassObject(person_class)
                    .Contains<ts::StringClass>(ts::NewClass<ts::StringClass>("surname")));
    ASSERT_FALSE(
        ts::ClassObject(person_class)
            .Contains<ts::PrimitiveClass<int>>(ts::NewClass<ts::PrimitiveClass<int>>("surname")));
    ASSERT_FALSE(ts::ClassObject(person_class)
                     .Contains<ts::StringClass>(ts::NewClass<ts::StringClass>("address")));
}

TEST(ClassStorage, ClassAddition) {
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
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, CONSOLE_LOGGER);
    database.AddClass(person_class);
    database.AddClass(coordinates_class);
    database.AddClass(city_class);
}

TEST(ClassStorage, PrintClasses) {
    auto database = db::Database(util::MakePtr<mem::File>("test.data"));
    database.PrintAllClasses();
}

TEST(ClassStorage, ClassRemoval) {
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
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kRead, CONSOLE_LOGGER);
    database.RemoveClass(person_class);
    database.RemoveClass(coordinates_class);
    // database.RemoveClass(city_class);
    database.PrintAllClasses();
}

TEST(Node, ReadWriteNode) {
    auto file = util::MakePtr<mem::File>("test.data", CONSOLE_LOGGER);
    file->Clear();
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));
    auto meta1 = db::Node(mem::kMagic, 0, ts::New<ts::Struct>(coords, 0., 0.));
    meta1.Write(file, 0);
    auto meta2 = db::Node(mem::kMagic, coords, file, 0);
    std::cerr << meta1.ToString() << std::endl;
    ASSERT_EQ(meta1.ToString(), meta2.ToString());
}

TEST(Node, NodeVarious) {
    auto file = util::MakePtr<mem::File>("test.data", CONSOLE_LOGGER);
    file->Clear();
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));
    auto string = ts::NewClass<ts::StringClass>("string");

    auto meta_coords = db::Node(mem::kMagic, 0, ts::New<ts::Struct>(coords, 0., 0.));
    meta_coords.Write(file, 0);
    file->Write(~mem::kMagic, 0);
    auto meta_coords_free = db::Node(mem::kMagic, coords, file, 0);
    file->Write<size_t>(0, 0);
    auto meta_coords_ivalid = db::Node(mem::kMagic, coords, file, 0);

    std::cerr << meta_coords.ToString() << std::endl;
    std::cerr << meta_coords_free.ToString() << std::endl;
    std::cerr << meta_coords_ivalid.ToString() << std::endl;

    auto meta_string = db::Node(mem::kMagic, 1, ts::New<ts::String>(string, "bebra"));
    meta_string.Write(file, 0);
    file->Write(~mem::kMagic, 0);
    auto meta_string_free = db::Node(mem::kMagic, string, file, 0);
    file->Write<size_t>(0, 0);
    auto meta_string_ivalid = db::Node(mem::kMagic, string, file, 0);

    std::cerr << meta_string.ToString() << std::endl;
    std::cerr << meta_string_free.ToString() << std::endl;
    std::cerr << meta_string_ivalid.ToString() << std::endl;
}

TEST(ValNodeStorage, NodeAddition) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(coords);

    database.AddNode(ts::New<ts::Struct>(coords, 1., 0.));
    database.AddNode(ts::New<ts::Struct>(coords, 0., 1.));
}

TEST(ValNodeStorage, NodeAddditionWithAllocation) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, CONSOLE_LOGGER);
    database.AddClass(coords);

    for (size_t i = 0; i < 200; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, 1., 0.));
        database.AddNode(ts::New<ts::Struct>(coords, 0., 1.));
    }
}

TEST(ValNodeStorage, PrintNodes) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, CONSOLE_LOGGER);
    database.AddClass(coords);

    for (size_t i = 0; i < 10; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, 13., 46.));
        database.AddNode(ts::New<ts::Struct>(coords, 60., 15.));
    }

    database.PrintNodesIf(coords, [](auto it) { return it.Id() % 2 == 0; });
}

TEST(ValNodeStorage, RemoveNodes) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(coords);

    for (size_t i = 0; i < 10; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, 13., 46.));
        database.AddNode(ts::New<ts::Struct>(coords, 60., 15.));
    }

    database.RemoveNodesIf(coords, [](auto it) { return it.Id() % 2 == 0; });
    database.PrintNodesIf(coords, []() { return true; });
}

TEST(ValNodeStorage, RemoveThenAddNodes) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(coords);

    for (size_t i = 0; i < 10; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, 13., 46.));
        database.AddNode(ts::New<ts::Struct>(coords, 60., 15.));
    }

    database.RemoveNodesIf(coords, [](auto it) { return it.Id() % 2 == 0; });

    for (size_t i = 0; i < 5; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, i * 1., -1. * i));
    }

    database.PrintNodesIf(coords, []() { return true; });
}

TEST(ValNodeStorage, FreeUnusedDataPages) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(coords);

    for (size_t i = 0; i < 1000; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, 13, 46));
    }

    database.RemoveNodesIf(coords, []() { return true; });
    database.PrintNodesIf(coords, []() { return true; });
}

TEST(ValNodeStorage, Select) {
    auto coords =
        ts::NewClass<ts::StructClass>("coords", ts::NewClass<ts::PrimitiveClass<double>>("lat"),
                                      ts::NewClass<ts::PrimitiveClass<double>>("lon"));

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, CONSOLE_LOGGER);
    database.AddClass(coords);

    for (size_t i = 0; i < 100; ++i) {
        database.AddNode(ts::New<ts::Struct>(coords, i * 10, 1000 - i));
    }

    database.PrintNodesIf(coords, [](db::ValNodeIterator it) {
        return it->Data<ts::Struct>()->GetField<ts::Primitive<double>>("lat")->Value() >
               it->Data<ts::Struct>()->GetField<ts::Primitive<double>>("lon")->Value();
    });
}

TEST(VarNodeStorage, NodeAddition) {
    auto name = ts::NewClass<ts::StringClass>("name");

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(name);

    database.AddNode(ts::New<ts::String>(name, "Greg"));
    database.AddNode(ts::New<ts::String>(name, "Gregory"));
    database.AddNode(ts::New<ts::String>(name, "Hyperb0rean"));
    database.PrintAllNodes(name);
}

TEST(VarNodeStorage, NodeAdditionWithAllocation) {
    auto name = ts::NewClass<ts::StringClass>("name");

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(name);

    for (size_t i = 0; i < 150; ++i) {
        database.AddNode(ts::New<ts::String>(name, "Greg"));
        database.AddNode(ts::New<ts::String>(name, "Gregory"));
        database.AddNode(ts::New<ts::String>(name, "Hyperb0rean"));
    }

    database.PrintAllNodes(name);
}

TEST(VarNodeStorage, NodeDeletion) {
    auto name = ts::NewClass<ts::StringClass>("name");

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(name);

    for (size_t i = 0; i < 50; ++i) {
        database.AddNode(ts::New<ts::String>(name, "Greg"));
        database.AddNode(ts::New<ts::String>(name, "Gregory"));
        database.AddNode(ts::New<ts::String>(name, "Hyperb0rean"));
        database.RemoveNodesIf(
            name, [](db::VarNodeIterator it) { return it->Data<ts::String>()->Value()[0] == 'G'; });
    }

    database.PrintAllNodes(name);
}

TEST(VarNodeStorage, NodeDeletionWithDeallocation) {
    auto name = ts::NewClass<ts::StringClass>("name");

    auto database =
        db::Database(util::MakePtr<mem::File>("test.data"), db::OpenMode::kWrite, DEBUG_LOGGER);
    database.AddClass(name);

    for (size_t i = 0; i < 150; ++i) {
        database.AddNode(ts::New<ts::String>(name, "Greg"));
        database.AddNode(ts::New<ts::String>(name, "Gregory"));
        if (i > 100) {
            database.AddNode(ts::New<ts::String>(name, "Hyperb0rean"));
        }
    }
    database.RemoveNodesIf(
        name, [](db::VarNodeIterator it) { return it->Data<ts::String>()->Value()[0] == 'G'; });
    database.PrintAllNodes(name);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}