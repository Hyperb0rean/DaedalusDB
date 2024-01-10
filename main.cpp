#include <iostream>

#include "io/io.h"

int main() {
    io::File f("test.txt");
    f.Write("Hello, world!\n", 14, 1);
    char buf[15];
    f.Read(buf, 14);
    std::cout << std::string{buf};
    return 0;
}