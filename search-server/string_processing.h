#pragma once

#include <string>
#include <set>
#include <vector>

template <typename StringContainer>
[[maybe_unused]] std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings);

[[maybe_unused]] std::vector<std::string> SplitIntoWords(std::string_view text);

//функция разделяет переданную строку на слова но работает со string_view
std::vector<std::string_view> SplitIntoWordsView(std::string_view str);

//template function realize
template <typename StringContainer>
[[maybe_unused]] std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}