#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <map>
#include <set>
#include <tuple>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <execution>

#include "document.h"
#include "string_processing.h"
#include "log_duration.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

using DocStatusType = std::tuple<std::vector<std::string_view>, DocumentStatus>;

class SearchServer {
public:
    SearchServer() = default;
    /*
     *  универсально создавать search-server, передавая в него при создании стоп-слова любыми
     *  возможными контейнерами (вектор, сет, стринг, инициализер-лист)
     */
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    //конструктор из строки стоп слов. Оставлен так как передача строки в конструктор string_view
    //приводит к вызову конструктора StringContainer не требующего преобразования, что неверно
    explicit SearchServer(const std::string& stop_words);
    //конструктор string_view
    explicit SearchServer(std::string_view stop_words);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
    [[nodiscard]] std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    [[nodiscard]] std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    /* Спринт 8. Параллелим поиск документов
     *
     */
    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& exec_policy, std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& exec_policy, std::string_view raw_query, DocumentStatus status) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& exec_policy, std::string_view raw_query) const;

    [[nodiscard]] int GetDocumentCount() const;

    /* Реализуйте метод MatchDocument:
     * В первом элементе кортежа верните все плюс-слова запроса, содержащиеся в документе.
     * Слова не должны дублироваться. Пусть они будут отсортированы по возрастанию.
     * Если документ не соответствует запросу (нет пересечений по плюс-словам или есть минус-слово),
     * вектор слов нужно вернуть пустым
     */
    [[nodiscard]] DocStatusType MatchDocument(std::string_view raw_query,
                                              int document_id) const;
    [[nodiscard]] DocStatusType MatchDocument(const std::execution::sequenced_policy&,
                                              std::string_view raw_query,
                                              int document_id) const;
    /* Параллельные алгоритмы. Урок 9: Параллелим методы поисковой системы 2/3
     * Реализуйте многопоточную версию метода MatchDocument в дополнение к однопоточной.
     */
    [[maybe_unused]] [[nodiscard]] DocStatusType MatchDocument(const std::execution::parallel_policy&,
                                                               std::string_view raw_query,
                                                               int document_id) const;
    /* Спринт 5
     * Откажитесь от метода GetDocumentId(int index) и вместо него определите методы begin и end. Они вернут итераторы.
     * Итератор даст доступ к id всех документов, хранящихся в поисковом сервере. Вы можете не разрабатывать собственный
     * итератор, а применить готовый константный итератор удобного контейнера.
     * Если begin и end определены корректно, появится возможность использовать упрощённую форму for с поисковым сервером
     */
    std::set<int>::iterator begin();
    std::set<int>::iterator end();
    [[nodiscard]] std::set<int>::const_iterator cbegin() const;
    [[nodiscard]] std::set<int>::const_iterator cend() const;
    /* Спринт 5
     * Разработайте метод получения частот слов по id документа:
     * Если документа не существует, возвратите ссылку на пустой map.
     */
    [[nodiscard]] const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    /* Спринт 5
     * Разработайте метод удаления документов из поискового сервера
     */
    /* Параллельные алгоритмы. Урок 9: Параллелим методы поисковой системы 1/3
     * Реализуйте многопоточную версию метода RemoveDocument в дополнение к однопоточной.
     * Как и прежде, в метод RemoveDocument может быть передан любой document_id
     */
    template <typename ExecutionPolicy>
    void RemoveDocument(const ExecutionPolicy& policy, int document_id);
    void RemoveDocument(int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    std::set<std::string> dictionary_;
    std::set<std::string_view> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    /* Спринт 5.
     * Добавлено для хранения частоты слов по документам
     */
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    //
    std::map<int, DocumentData> documents_;
    /* Спринт 5.
     * Vector refactor to set for ease erase
     */
    std::set<int> document_ids_;

    [[nodiscard]] bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    [[nodiscard]] const std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    [[nodiscard]] QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    /*
     * Разберем запрос на структуру пллюс слова и минус слова
     */
    template <typename ExecutionPolicy>
    [[nodiscard]] Query ParseQuery(std::string_view text, const ExecutionPolicy& exec_policy) const;

    [[nodiscard]] double ComputeWordInverseDocumentFreq(std::string_view word) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy& exec_policy, const Query& query,
                                           DocumentPredicate document_predicate) const;
};

/*
 *  универсально создавать search-server, передавая в него при создании стоп-слова любыми
 *  возможными контейнерами (вектор, сет, стринг, инициализер-лист)
 */
template <typename StringContainer>
 SearchServer::SearchServer(const StringContainer& stop_words) {
    using namespace std::string_literals;
    if (std::any_of(stop_words.cbegin(), stop_words.cend(),
                    [](std::string_view word){return !SearchServer::IsValidWord(word);}))
    {
        throw std::invalid_argument("Недопустимые символы (с кодами от 0 до 31) в стоп словах"s);
    }
    for (std::string_view word : stop_words) {
        if (!word.empty()) {
            auto [it_word, _] = dictionary_.emplace(word);
            stop_words_.emplace(*it_word);
        }
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& exec_policy,
                                       const std::string_view raw_query,
                                       DocumentPredicate document_predicate) const {
    const SearchServer::Query query = SearchServer::ParseQuery(raw_query, std::execution::seq);
    auto matched_documents = SearchServer::FindAllDocuments(exec_policy, query, document_predicate);

    std::sort(exec_policy, matched_documents.begin(), matched_documents.end(),
         [](const Document& lhs, const Document& rhs) {
             if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                 return lhs.rating > rhs.rating;
             } else {
                 return lhs.relevance > rhs.relevance;
             }
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& exec_policy,
                                                     const std::string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(exec_policy, raw_query, [status]([[maybe_unused]] int document_id,
                                                                                   DocumentStatus lstatus, [[maybe_unused]] int rating){
        return lstatus == status;});
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& exec_policy,
                                                     const std::string_view raw_query) const {
    return SearchServer::FindTopDocuments(exec_policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy& exec_policy, const SearchServer::Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(1000);
    //сделаем заглушку до распарралеливания
    //так как в параллельной версии тут дубли
    //пока парсинг был без дублей seq версия
    //
    const auto plus_words_predicate = [this, &document_to_relevance, &document_predicate](std::string_view word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        const double inverse_document_freq = SearchServer::ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq]: word_to_document_freqs_.at(word)) {
            const auto &document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
            }
        }
    };
    std::for_each(exec_policy, query.plus_words.begin(), query.plus_words.end(), plus_words_predicate);

    const auto minus_word_predicate =
            [this, &document_to_relevance](std::string_view word) {
                if (word_to_document_freqs_.count(word) == 0) {
                    return;
                }
                for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                    document_to_relevance.Erase(document_id);
                }
            };
    std::for_each(exec_policy, query.minus_words.begin(), query.minus_words.end(), minus_word_predicate);

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.emplace_back(
                                            document_id,
                                            relevance,
                                            documents_.at(document_id).rating
                                    );
    }
    return matched_documents;
}

/* Параллельные алгоритмы. Урок 9: Параллелим методы поисковой системы
* Реализуйте многопоточную версию метода RemoveDocument в дополнение к однопоточной.
* Как и прежде, в метод RemoveDocument может быть передан любой document_id
*/
template <typename ExecutionPolicy>
void SearchServer::RemoveDocument(const ExecutionPolicy& policy, int document_id) {
    if (!documents_.count(document_id)) {return;}

    documents_.erase(document_id); //Complexity: log(c.size()) + c.count(key)
    document_ids_.erase(document_id); //Complexity: log(c.size()) + c.count(key)

    const auto& words_of_doc = document_to_word_freqs_.at(document_id);
    std::vector<std::string_view> words_to_erase(words_of_doc.size());
    std::transform(policy, words_of_doc.begin(), words_of_doc.end(),
                   words_to_erase.begin(),
                   [](const auto& words_freq){ return words_freq.first;});

    std::for_each(policy, words_to_erase.begin(), words_to_erase.end(),
                  [this, document_id](const auto& word){word_to_document_freqs_.at(word).erase(document_id);});

    document_to_word_freqs_.erase(document_id); //Complexity: log(c.size()) + c.count(key)
}

/* Версия параллельной обработки
 * Перешли на вектора но в данной версии не работаем с уникальностью
 * Разберем запрос на структуру плюс слова и минус слова
 */
template <typename ExecutionPolicy>
SearchServer::Query SearchServer::ParseQuery(std::string_view text, [[maybe_unused]] const ExecutionPolicy& exec_policy) const {
    SearchServer::Query query;
    for (std::string_view word : SplitIntoWordsView(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                //контейнер вектор - но в данной версии не работаем с уникальностью
                query.minus_words.push_back(query_word.data);
            } else {
                //контейнер вектор - но в данной версии не работаем с уникальностью
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    //для непараллельной версии нужно удалить дубли, так как дальше они не обрабатываются
    if constexpr(std::is_same_v<std::execution::sequenced_policy, ExecutionPolicy>) {
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
    }
    return query;
}