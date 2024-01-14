#include <iostream>

#include "db_struct/database.h"

struct A {
public:
    virtual ~A() {
    }
    virtual void Foo() {
        std::cout << "A";
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
    auto file = std::make_shared<mem::File>("database.data");

    file->Write<int>({4, 5, 6});
    auto five = file->Read<int>(4);

    B test;
    test.value = 1;
    test.Foo();
    // 1
    file->Write<B>(test, 0, 8);
    B test2 = file->Read<B>(0, 8);
    test2.Foo();
    // 1

    std::cout << five;
    return 0;
}
