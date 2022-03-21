#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <deque>

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
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

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
            : id(id)
            , relevance(relevance)
            , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) {
        if (std::any_of(stop_words.cbegin(), stop_words.cend(), [](const string& word){return !IsValidWord(word);}))
        {
            throw invalid_argument("Недопустимые символы (с кодами от 0 до 31) в стоп словах"s);
        }
        for (const string& word : stop_words) {
            if (word != ""s) {
                stop_words_.insert(word);
            }
        }
    }
    
    explicit SearchServer(const string& stop_words) : SearchServer(SplitIntoWords(stop_words)) {}

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw invalid_argument("Отрицательный id или id ранее добавленного документа"s);
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        document_ids_.push_back(document_id);
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus lstatus, int rating){
            (void)(document_id);
            (void)(rating);
            return lstatus == status;});
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    int GetDocumentId(int index) const {
        if (index >= 0 && index < GetDocumentCount()) {
            return document_ids_[index];
        }
        throw out_of_range("Индекс документа за пределами допустимого диапазона (0; количество документов)"s);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> document_ids_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    const vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Недопустимые символы (с кодами от 0 до 31) в тексте добавляемого документа"s);
            }
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        // Empty result by initializing it with default constructed QueryWord
        if (text.empty()) {
            throw invalid_argument("ParseQueryWord: Текст запроса некорректен"s);
        }
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
            throw invalid_argument("ParseQueryWord: Текст запроса некорректен"s);
        }

        return QueryWord{text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

     Query ParseQuery(const string& text) const {
        // Empty result by initializing it with default constructed Query
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate filter_function) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (filter_function(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                                                document_id,
                                                relevance,
                                                documents_.at(document_id).rating
                                        });
        }
        return matched_documents;
    }

};

template <typename Iterator>
class Paginator {
    // тело класса
public:
    Paginator(Iterator It_begin, Iterator It_end, int PageSize) {
        _it_begin = It_begin;
        _it_end = It_end;
        _size = PageSize;

        //devide contaner into pages
        int pages_count = distance(It_begin, It_end) / PageSize;
        //add full pages except last page
        for (int page = 0; page != pages_count; ++page) {
            auto const _it = _it_begin;
            advance(_it_begin, _size);
            _pages.push_back(pair{_it, _it_begin});
        }
        //add last page
        if (_it_begin != _it_end) {
            _pages.push_back(pair{_it_begin, _it_end});
        }
    }

    auto begin() const {
        return _pages.begin();
    }

    auto end() const {
        return _pages.end();
    }

    int size() const {
        return _pages.size();
    }
private:
    Iterator _it_begin;
    Iterator _it_end;
    int _size = 0;
    vector<pair<Iterator, Iterator>> _pages;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename Element>
ostream& operator<<(ostream& out, const vector<Element>& container) {
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
ostream& operator<<(ostream& out, const map<T, U>& container) {
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
ostream& operator<<(ostream& out, const set<Element>& container) {
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

ostream& operator<<(ostream& out, const Document& document)  {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s;
    return out;
}

template <typename Iterator>
ostream& operator<<(ostream& out, const pair<Iterator, Iterator>& container) {
    for (auto document = container.first; document != container.second; ++document) {
        out << *document;
    }
    return out;
}

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
        // напишите реализацию
        : search_server_(search_server),
        time_(0) {

    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
        // напишите реализацию
        auto matched_documents = search_server_.FindTopDocuments(raw_query, document_predicate);
        PutRequestIntoQueue(raw_query, matched_documents);
        return matched_documents;
    }

    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status) {
        // напишите реализацию
        auto matched_documents = search_server_.FindTopDocuments(raw_query, status);
        PutRequestIntoQueue(raw_query, matched_documents);
        return matched_documents;
    }

    vector<Document> AddFindRequest(const string& raw_query) {
        // напишите реализацию
        auto matched_documents = search_server_.FindTopDocuments(raw_query);
        PutRequestIntoQueue(raw_query, matched_documents);
        return matched_documents;
    }

    int GetNoResultRequests() const {
        // напишите реализацию
        return count_if(requests_.begin(), requests_.end(), [] (QueryResult result) {return result.result.empty();});
    }
private:
    struct QueryResult {
        // определите, что должно быть в структуре
        int timestamp;
        const string& raw_query;
        const vector<Document> result;
    };
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    // возможно, здесь вам понадобится что-то ещё
    const SearchServer& search_server_;
    int time_;
    /*
     В момент появления нового запроса:
    Увеличьте время на одну секунду (запросы приходят раз в секунду);
    Удалите из дека все запросы, которые успели устареть;
    Не забудьте во время удаления уменьшить количество запросов с пустым вектором ответов, если нужно;
    Добавьте новый запрос в дек и обновите количество запросов без результатов поиска.
     */
    void PutRequestIntoQueue(const string& raw_query, const vector<Document>& result) {
        ++time_;
        if (time_ > min_in_day_) {
            //time_ = 0;
            requests_.pop_front();
        }
        requests_.push_back(QueryResult{time_, raw_query, result});
    }
};


// -------- Начало модульных тестов поисковой системы ----------
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "Assert("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestAddedDocumentMustBeFind() {
    /*
     * Добавление документов. Добавленный документ должен находиться по поисковому запросу,
     * который содержит слова из документа.
     */
    const int doc_id = 43;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что если документ не добавлен его не найти
    {
        SearchServer server;
        const auto found_docs = server.FindTopDocuments("in the city"s);
        ASSERT(!found_docs.size());
    }
    //затем убеждаемся что документ добавлен и найдет по словам
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("the city"s);
        ASSERT(found_docs.size());
        const Document &doc0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc0.id, doc_id, "Added document must be found by query words"s);
    }

}

void TestMinusWordDocumentsExclude() {
    /*
     * Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса,
     * не должны включаться в результаты поиска.
     */
    const int doc_id = 44;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city -cat"s);
        ASSERT_HINT(!found_docs.size(), "Documents with minus words must be exclude from search request"s);
    }
}

void TestDocumentsMatching() {
    /*
     * Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из
     * поискового запроса, присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову,
     * должен возвращаться пустой список слов.
     */
    const int doc_id = 45;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [found_docs_word, status] = server.MatchDocument("city"s, doc_id);
        ASSERT_EQUAL(found_docs_word, vector<string>({"city"s}));
    }
    //
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [found_docs_word, status] = server.MatchDocument("city -the"s, doc_id);
        ASSERT_HINT(!found_docs_word.size(), "If minus words match - list of words must be empty"s);
    }
}

void TestSortByRelevance() {
    /*
     * Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты
     * должны быть отсортированы в порядке убывания релевантности.
     */
    const int doc_id = 46;
    const string content = "cat in the city"s;
    const int doc_id1 = 461;
    const string content1 = "dog in the house"s;
    const int doc_id2 = 462;
    const string content2 = "horse in the city"s;
    const int doc_id3 = 463;
    const string content3 = "dog in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat city -dog"s);
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT_HINT(found_docs[0].relevance > found_docs[1].relevance, "Documents must be sort be relevance descending"s);
    }

}

void TestRatingCalculation() {
    /*
     * Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
     */
    const int doc_id = 47;
    const string content = "cat in the city"s;
    vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL_HINT(found_docs[0].rating, 2, "Rating must be calculated as average"s);
    }
    ratings = {-1, -2, -3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL_HINT(found_docs[0].rating, -2, "Rating must be calculated as average"s);
    }
    ratings = {1, -2, -3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL_HINT(found_docs[0].rating, -1, "Rating must be calculated as average"s);
    }
}

void TestFilterByPredicate() {
    /*
     * Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
     */
    const int doc_id = 48;
    const string content = "cat in the city"s;
    const int doc_id1 = 481;
    const string content1 = "dog in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::BANNED, ratings);

        const auto found_docs = server.FindTopDocuments("city"s,
                                                        [](int document_id, DocumentStatus lstatus, int rating){
                                                            (void)(document_id);
                                                            (void)(rating);
                                                            return (lstatus == DocumentStatus::ACTUAL || lstatus == DocumentStatus::BANNED);
                                                        }
        );
        ASSERT_EQUAL(found_docs.size(), 2);
    }
    //
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::BANNED, ratings);

        const auto found_docs = server.FindTopDocuments("city"s,
                                                        [](int document_id, DocumentStatus lstatus, int rating){
                                                            (void)(document_id);
                                                            (void)(rating);
                                                            return (lstatus == DocumentStatus::BANNED);
                                                        }
        );
        ASSERT_EQUAL_HINT(found_docs.size(), 1, "Incorrect filtering by predicat"s);
    }
}

void TestFindByStatus() {
    /*
     * Поиск документов, имеющих заданный статус.
     */
    const int doc_id = 49;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 1);
    }
    //
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::BANNED);
        ASSERT_EQUAL_HINT(found_docs.size(), 0, "Incorrect filtering by status of document"s);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL_HINT(found_docs.size(), 0, "Incorrect filtering by status of document"s);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL_HINT(found_docs.size(), 0, "Incorrect filtering by status of document"s);
    }
}

void TestRelevanceCalc() {
    /*
     * Корректное вычисление релевантности найденных документов.
     */
    const int doc_id = 50;
    const string content = "cat in the city"s;
    const int doc_id1 = 51;
    const string content1 = "dog in the house"s;
    const int doc_id2 = 52;
    const string content2 = "horse in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
        auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT_HINT(abs(found_docs[0].relevance - 0.1013662770270411) < EPSILON, "Incorrect relevance calculating"s);
        found_docs = server.FindTopDocuments("house"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_HINT(abs(found_docs[0].relevance - 0.27465307216702745) < EPSILON, "Incorrect relevance calculating"s);
        found_docs = server.FindTopDocuments("in"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 3);
        ASSERT_HINT(abs(found_docs[0].relevance - 0) < EPSILON, "Incorrect relevance calculating"s);
    }

}

template <typename T>
void RunTestImpl(T& func, const string& name) {
    func();
    cerr << name <<" OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddedDocumentMustBeFind);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWordDocumentsExclude);
    RUN_TEST(TestDocumentsMatching);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestRatingCalculation);
    RUN_TEST(TestFilterByPredicate);
    RUN_TEST(TestFindByStatus);
    RUN_TEST(TestRelevanceCalc);
}

// --------- Окончание модульных тестов поисковой системы -----------

// ==================== для примера =========================
void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
                 const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const exception& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const exception& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const exception& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;

//    SearchServer search_server("и в на"s);
//
//    AddDocument(search_server, 1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
//    AddDocument(search_server, 1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
//    AddDocument(search_server, -1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
//    AddDocument(search_server, 3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
//    AddDocument(search_server, 4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 1, 1});
//
//    FindTopDocuments(search_server, "пушистый -пёс"s);
//    FindTopDocuments(search_server, "пушистый --кот"s);
//    FindTopDocuments(search_server, "пушистый -"s);
//
//    MatchDocuments(search_server, "пушистый пёс"s);
//    MatchDocuments(search_server, "модный -кот"s);
//    MatchDocuments(search_server, "модный --пёс"s);
//    MatchDocuments(search_server, "пушистый - хвост"s);

//    SearchServer search_server("and with"s);
//
//    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
//    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
//    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
//    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
//    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});
//
//    //const auto search_results = search_server.FindTopDocuments("curly dog"s);
//    const auto search_results = search_server.FindTopDocuments("naughty"s);
//    int page_size = 2;
//    const auto pages = Paginate(search_results, page_size);
//
//    // Выводим найденные документы по страницам
//    for (auto page = pages.begin(); page != pages.end(); ++page) {
//        cout << *page << endl;
//        cout << "Page break"s << endl;
//    }

    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    return 0;
}