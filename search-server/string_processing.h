#pragma once

#include <string>
#include <set>
#include <vector>

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings);

std::vector<std::string> SplitIntoWords(const std::string_view text);

//функция разделяет переданную строку на слова но работает со string_view
std::vector<std::string_view> SplitIntoWordsView(std::string_view str);

//template function realize
template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}