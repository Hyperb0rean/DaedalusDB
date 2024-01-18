#include <iostream>

#include "database.h"

int main() {

    auto file = std::make_shared<mem::File>("database.data");   
    return 0;
}
