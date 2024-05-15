#include "test.hpp"

TEST(TypeSystem, SimpleReadWrite) {
    auto file = util::MakePtr<mem::File>("test.data");
    file->Clear();
    auto name = ts::NewClass<ts::StringClass>("name");
    auto node = ts::New<ts::String>(name, "Greg"s);
    node->Write(file, 0);
    file->Write("Cool", 4, 0, 4);

    ASSERT_EQ("name: \"Greg\"", node->ToString());
    node->Read(file, 0);
    ASSERT_EQ("name: \"Cool\"", node->ToString());
}

TEST(TypeSystem, InvalidClasses) {
    ASSERT_THROW(auto name = ts::NewClass<ts::StringClass>("name_"), error::TypeError);
    ASSERT_THROW(auto name = ts::NewClass<ts::StringClass>("n@me"), error::TypeError);
    ASSERT_THROW(auto name = ts::NewClass<ts::StringClass>("<name>"), error::TypeError);
}

TEST(TypeSystem, ReadWrite) {
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

TEST(TypeSystem, SafeNew) {
    auto file = util::MakePtr<mem::File>("test.data");
    file->Clear();
    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    ASSERT_THROW(auto node = ts::New<ts::Struct>(person_class, "Greg"s, "Sosnovtsev"s),
                 error::BadArgument);
}

TEST(TypeSystem, DefaultNew) {
    auto file = util::MakePtr<mem::File>("test.data");
    file->Clear();
    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<bool>>("male"));

    auto node = ts::DefaultNew<ts::Struct>(person_class);

    ASSERT_EQ("person: { name: \"\", surname: \"\", age: 0, male: false }", node->ToString());
}

TEST(TypeSystem, ReadNew) {
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

TEST(TypeSystem, TypeDump) {
    auto file = util::MakePtr<mem::File>("test.data");
    file->Clear();

    auto person_class = ts::NewClass<ts::StructClass>(
        "person", ts::NewClass<ts::StringClass>("name"), ts::NewClass<ts::StringClass>("surname"),
        ts::NewClass<ts::PrimitiveClass<int>>("age"),
        ts::NewClass<ts::PrimitiveClass<uint64_t>>("money"));

    ts::ClassObject(person_class).Write(file, 1488);
    ASSERT_EQ(ts::ClassObject(person_class).ToString(),
              "_struct@person_<_string@name__string@surname__int@age__unsignedlong@money_>_");

    ts::ClassObject read_class;
    read_class.Read(file, 1488);
    ASSERT_EQ(read_class.ToString(), ts::ClassObject(person_class).ToString());
}

TEST(TypeSystem, Metadata) {
    // junk
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
