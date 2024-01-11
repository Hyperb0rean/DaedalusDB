#include <iostream>

#include "io/io.h"

int main() {
    io::File f("test.txt");
    f.Write(std::string{"Hello, world!\n"}, 10);
    std::cout << f.Read(0, 15);
    return 0;
}