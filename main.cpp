#include <iostream>

#include "db_struct/database.h"

struct A {
public:
    virtual ~A() {
    }
    virtual void Foo() {
        throw error::NotImplemented("lol");
    }
};

struct B : public A {
    int value;

public:
    ~B() = default;
    void Foo() override {
        std::cout << value << '\n';
    }
};

int main() {
    // auto file = std::make_unique<mem::File>("test.txt");
    // B test;
    // test.value = 1;
    // test.Foo();
    // // 1
    // file->Write<B>(test, 0, 8);
    // B test2 = file->Read<B>(0, 8);
    // test2.Foo();
    // // 1

    auto file = std::make_unique<mem::File>("database.data");
    auto coords = types::StructType("coords");
    coords.AddField(types::PrimitiveType<double>("lat"));
    coords.AddField(types::PrimitiveType<double>("lon"));

    auto city = std::make_shared<types::StructType>("city");
    city->AddField(types::PrimitiveType<std::string>("name"));
    city->AddField(coords);

    auto node = types::Struct("person");
    node.AddFieldValue(types::Primitive<std::string>("name", "Greg"));
    node.AddFieldValue(types::Primitive<std::string>("surname", "Sosnovtsev"));
    node.AddFieldValue(types::Primitive<int>("age", 19));

    node.Write(file, 0);
    file->Write("Cool", 4, 0, 4);
    file->Write(20, 22);

    std::cerr << node.ToString() << '\n';
    // person: { name: "Greg", surname: "Sosnovtsev" }
    node.Read(file, 0);
    std::cerr << node.ToString();
    // person: { name: "Cool", surname: "Sosnovtsev" }

    return 0;
}
