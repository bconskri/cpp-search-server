#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>

using namespace std::string_literals;

std::string ReadLine();

[[maybe_unused]] int ReadLineWithNumber();

template <typename Element>
std::ostream& operator<<(std::ostream& out, const std::vector<Element>& container) {
    bool is_first = true;
    for (const auto& element : container) {
        if (!is_first) {
            out << ", "s;
        } else {
            out << "[";
        }
        is_first = false;
        out << element;
    }
    out << "]";
    return out;
}

template <typename T, typename U>
std::ostream& operator<<(std::ostream& out, const std::map<T, U>& container) {
    bool is_first = true;
    for (const auto& [key, value] : container) {
        if (!is_first) {
            out << ", "s;
        } else {
            out << "{";
        }
        is_first = false;
        out << key << ": " << value;
    }
    out << "}";
    return out;
}

template <typename Element>
std::ostream& operator<<(std::ostream& out, const std::set<Element>& container) {
    bool is_first = true;
    for (const auto& element : container) {
        if (!is_first) {
            out << ", "s;
        } else {
            out << "{";
        }
        is_first = false;
        out << element;
    }
    out << "}";
    return out;
}