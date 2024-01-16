#include <cassert>
#include <iostream>

#include "database.h"

struct A {
public:
    virtual ~A() {
    }
    virtual int Foo() {
        throw error::NotImplemented("lol");
    }
};

struct B : public A {
    int value;

public:
    ~B() = default;
    int Foo() override {
        return value;
    }
};

int main() {
    auto file = std::make_unique<mem::File>("test.txt");
    B test;
    test.value = 1;
    assert(test.Foo() == 1);
    file->Write<B>(test, 0, 8);
    B test2 = file->Read<B>(0, 8);
    assert(test.Foo() == 1);
    return 0;
}
