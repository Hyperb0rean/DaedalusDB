#include <iostream>

#include "database.hpp"

int main() {

    auto file = std::make_shared<mem::File>("database.data");
    return 0;
}
