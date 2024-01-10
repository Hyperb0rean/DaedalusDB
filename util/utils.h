#pragma once
#include <chrono>
#include <cstring>
#include <ctime>
#include <string>

const std::string GetCurrentTime() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return buf;
}