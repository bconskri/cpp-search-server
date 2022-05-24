#include "search_server.h"

//public:
SearchServer::SearchServer(const std::string& stop_words) : SearchServer::SearchServer(SplitIntoWordsView(stop_words)) {}

SearchServer::SearchServer(const std::string_view stop_words) : SearchServer::SearchServer(SplitIntoWordsView(stop_words)) {}

void SearchServer::AddDocument(int document_id, const std::string_view document,
                               DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Отрицательный id или id ранее добавленного документа"s);
    }
    const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view word : words) {
        auto [it_word, yes] = dictionary_.emplace(word);
        word_to_document_freqs_[*it_word][document_id] += inv_word_count;
        /*
         * Спринт 5. Добавлено хранение частоты слов по документам
         */
        document_to_word_freqs_[document_id][*it_word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{SearchServer::ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query, status);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

/* Реализуйте метод MatchDocument:
 * В первом элементе кортежа верните все плюс-слова запроса, содержащиеся в документе.
 * Слова не должны дублироваться. Пусть они будут отсортированы по возрастанию.
 * Если документ не соответствует запросу (нет пересечений по плюс-словам или есть минус-слово),
 * вектор слов нужно вернуть пустым
 */
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query,
                                                                                      int document_id) const {
    return SearchServer::MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&,
                                                                        const std::string_view raw_query, int document_id) const {
    //разберем запрос на структуру плюс и минус слов
    const Query query = SearchServer::ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;
    //если документ содержит минус слово, то документ нам не подходит
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) != 0 &&
            word_to_document_freqs_.at(word).count(document_id)) {
            return {matched_words, documents_.at(document_id).status};
        }
    }
    //для каждого плюс слова найдем документ, который его содержит
    //и запомним плюс слово в таком случае
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) != 0 &&
            word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

std::set<int>::iterator SearchServer::begin() {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::cbegin() const {
    return document_ids_.cbegin();
}

std::set<int>::iterator SearchServer::end() {
    return document_ids_.end();
}

std::set<int>::const_iterator SearchServer::cend() const {
    return document_ids_.cend();
}

const std::map<std::string_view, double> &SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double> empty_map;
    auto it = document_to_word_freqs_.find(document_id); //Complexity: Log in the size of the container.
    if (it != document_to_word_freqs_.end()) {
        return it -> second;
    }
    return empty_map;
}

void SearchServer::RemoveDocument(int document_id) {
    if (!documents_.count(document_id)) {return;}

    documents_.erase(document_id); //Complexity: log(c.size()) + c.count(key)
    document_ids_.erase(document_id); //Complexity: log(c.size()) + c.count(key)

    for (auto [word, freq] : document_to_word_freqs_.at(document_id)) { //W
        word_to_document_freqs_.at(word).erase(document_id); //Complexity: Log in the size of the container.
    }

    document_to_word_freqs_.erase(document_id); //Complexity: log(c.size()) + c.count(key)
}

//private:

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(std::string{word}) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

const std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWordsView(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Недопустимые символы (с кодами от 0 до 31) в тексте добавляемого документа"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    // Empty result by initializing it with default constructed QueryWord
    if (text.empty()) {
        throw std::invalid_argument("ParseQueryWord: Текст запроса некорректен"s);
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw std::invalid_argument("ParseQueryWord: Текст запроса некорректен"s);
    }

    return SearchServer::QueryWord{text, is_minus, IsStopWord(text)};
}

/*
 * Разберем запрос на структуру пллюс слова и минус слова
 */
SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    SearchServer::Query query;
    for (const std::string_view word : SplitIntoWordsView(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            } else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    //так как контейнер вектор - нужно следить за уникальностью значений
    if  ((query.minus_words.end() - query.minus_words.begin()) > 0) {
        std::sort(query.minus_words.begin(), query.minus_words.end());
        auto itm = std::unique(query.minus_words.begin(), query.minus_words.end());
        query.minus_words.resize(std::distance(query.minus_words.begin(), itm));
    }
    //так как контейнер вектор - нужно следить за уникальностью значений
    if  ((query.plus_words.end() - query.plus_words.begin()) > 0) {
        std::sort(query.plus_words.begin(), query.plus_words.end());
        auto itp = std::unique(query.plus_words.begin(), query.plus_words.end());
        query.plus_words.resize(std::distance(query.plus_words.begin(), itp));
    }
    //
    return query;
}

/* Версия параллельной обработки
 * Перешли на вектора но в данной версии не работаем с уникальностью
 * Разберем запрос на структуру плюс слова и минус слова
 */
SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy&, const std::string_view text) const {
    SearchServer::Query result;
    for (const std::string_view word : SplitIntoWordsView(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                //контейнер вектор - но в данной версии не работаем с уникальностью
                result.minus_words.push_back(query_word.data);
            } else {
                //контейнер вектор - но в данной версии не работаем с уникальностью
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

/* Параллельные алгоритмы. Урок 9: Параллелим методы поисковой системы 2/3
* Реализуйте многопоточную версию метода MatchDocument в дополнение к однопоточной.
*/
[[maybe_unused]] std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&,
                                                                                      const std::string_view raw_query,
                                                                                      int document_id) const {
    const Query query = ParseQuery(std::execution::par, raw_query);
    std::vector<std::string_view> matched_words;
    //если хоть одно минус слово встречается в документе - возвращаем пустой матчинг
    if (any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
               [this, document_id](std::string_view word) {
                   return (word_to_document_freqs_.count(word) > 0 &&
                           word_to_document_freqs_.at(word).count(document_id) > 0);
               })) {
        return {matched_words, documents_.at(document_id).status};
    }
    //для каждого плюс слова если найдем документ, который его содержит
    //то запомним плюс слово в таком случае
    matched_words.resize(query.plus_words.size());
    auto it = std::copy_if(std::execution::par,
                           query.plus_words.begin(),
                           query.plus_words.end(),
                           matched_words.begin(),
                           [this, document_id](const std::string_view word) {
                               return (word_to_document_freqs_.count(word) > 0 &&
                                       word_to_document_freqs_.at(word).count(document_id) > 0);
                           }
    );
    //
    std::sort(std::execution::par, matched_words.begin(), it);
    it = std::unique(std::execution::par, matched_words.begin(), it);
    matched_words.erase(it, matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}