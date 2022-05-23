//
// Created by Родион Каргаполов on 13.05.2022.
//
/* Версия для распараллеливания
 * Разберем запрос на структуру пллюс слова и минус слова
 */
template <typename ExecutionPolicy>
SearchServer::Query SearchServer::ParseQuery(ExecutionPolicy policy, const std::string& text) const {
    // Empty result by initializing it with default constructed Query
    SearchServer::Query result;
    std::vector<std::string> text_words = SplitIntoWords(text);

    //заберем только минус слова
    std::copy_if(policy,
                 text_words.begin(),text_words.end(),
                 std::back_inserter(result.minus_words),
                 [this](const std::string& word){
                     const SearchServer::QueryWord query_word = SearchServer::ParseQueryWord(word);
                     return !query_word.is_stop && query_word.is_minus;
                 }
    );
    //так как минус слова содержат лидирующий минус то его надо отрезать
    std::transform(policy,
                   result.minus_words.begin(),result.minus_words.end(),
                   result.minus_words.begin(),
                   [result](const std::string& word){return word.substr(1);});

    //заберем только плюс слова
    std::copy_if(policy,
                 text_words.begin(),text_words.end(),
                 std::back_inserter(result.plus_words),
                 [this](const std::string& word){
                     const SearchServer::QueryWord query_word = SearchServer::ParseQueryWord(word);
                     return !query_word.is_stop && !query_word.is_minus;
                 }
    );

    return result;
}