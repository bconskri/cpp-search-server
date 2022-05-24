#include "read_input_functions.h"

std::string ReadLine() {
    std::string s;
    getline(std::cin, s);
    return s;
}

[[maybe_unused]] int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}