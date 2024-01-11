#include <iostream>

#include "io/io.h"

int main() {
    io::File f("test.txt");
    struct Word {
        char w[5];
    };
    f.Write(std::string{"Hello, world!\n"});
    Word p = f.Read<Word>();
    for (size_t i = 0; i < 5; ++i) {
        std::cout << p.w[i];
    }
    f.Write<int>(150, 60);
    int val = f.Read<int>(60);
    std::cout << std::endl << val << ' ' << f.GetSize();
    return 0;
}