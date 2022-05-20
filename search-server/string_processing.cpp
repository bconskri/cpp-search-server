#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string_view text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

/* Урок 9. Совершенствуем парсинг слов
 * В прошлом уроке для парсинга строки вы использовали переменную pos,
 * которая указывала на позицию начала слова. Избавьтесь от этой переменной и
 * начните перемещать начало самого string_view.
 */
std::vector<std::string_view> SplitIntoWordsView(std::string_view str) {
    std::vector<std::string_view> result;
    str.remove_prefix(std::min(str.find_first_not_of(' '), str.size()));
    const std::string_view::size_type pos_end = str.npos;

    while (str.size() > 0) {
        std::string_view::size_type space = str.find(' ');
        result.push_back(space == pos_end ? str.substr(0, str.size()) : str.substr(0, space));
        str.remove_prefix(std::min(str.find_first_not_of(' ', space), str.size()));
    }

    return result;
}