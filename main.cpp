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
    auto file = std::make_unique<mem::File>("database.data");
    B test;
    test.value = 1;
    test.Foo();
    // 1
    file->Write<B>(test, 0, 8);
    B test2 = file->Read<B>(0, 8);
    test2.Foo();
    // 1
    return 0;
}
