#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */
const int MAX_RESULT_DOCUMENT_COUNT = 5;

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
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
                           DocumentData{
                                   ComputeAverageRating(ratings),
                                   status
                           });
    }

    template <typename Filter>
    vector<Document> FindTopDocuments(const string& raw_query, Filter filter_function) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, filter_function);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
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

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
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
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
                text,
                is_minus,
                IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
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

    template <typename Filter
    >
    vector<Document> FindAllDocuments(const Query& query, Filter filter_function) const {
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

// -------- Начало модульных тестов поисковой системы ----------
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
        SearchServer server;
        server.SetStopWords("in the"s);
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
        ASSERT_EQUAL_HINT(found_docs[0].id, 46, "Documents must be sort be relevance descending"s);
    }

}

void TestRatingCalculation() {
    /*
     * Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
     */
    const int doc_id = 47;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL_HINT(found_docs[0].rating, 2, "Rating must be calculated as average"s);
    }

}

void TestFilterByPredicat() {
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
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT_EQUAL_HINT(found_docs[0].relevance, 0.1013662770270411, "Incorrect relevance calculating"s);
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
    RUN_TEST(TestFilterByPredicat);
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

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;

    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }

    cout << "ACTUAL:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s,
                                                                   [](int document_id, DocumentStatus status, int rating) {
                                                                                    (void)(document_id);
                                                                                    (void)(rating);
                                                                                    return status == DocumentStatus::ACTUAL; })) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s,
                                                                   [](int document_id, DocumentStatus status, int rating) {
                                                                                (void)(status);
                                                                                (void)(rating);
                                                                                return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    return 0;
}