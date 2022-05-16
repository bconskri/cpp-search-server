//
// Created by Родион Каргаполов on 22.03.2022.
//
#include "tests.h"
#include "search_server.h"

// -------- Начало модульных тестов поисковой системы ----------
void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
                const std::string& hint) {
    if (!value) {
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "Assert("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        std::abort();
    }
}

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
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
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
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
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
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
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [found_docs_word, status] = server.MatchDocument("city"s, doc_id);
        ASSERT_EQUAL(found_docs_word, std::vector<std::string_view>({"city"s}));
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
    const std::string content = "cat in the city"s;
    const int doc_id1 = 461;
    const std::string content1 = "dog in the house"s;
    const int doc_id2 = 462;
    const std::string content2 = "horse in the city"s;
    const int doc_id3 = 463;
    const std::string content3 = "dog in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat city -dog"s);
        ASSERT_EQUAL(found_docs.size(), 2u);
        ASSERT_HINT(found_docs[0].relevance > found_docs[1].relevance, "Documents must be sort be relevance descending"s);
    }

}

void TestRatingCalculation() {
    /*
     * Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
     */
    const int doc_id = 47;
    const std::string content = "cat in the city"s;
    std::vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL_HINT(found_docs[0].rating, 2, "Rating must be calculated as average"s);
    }
    ratings = {-1, -2, -3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL_HINT(found_docs[0].rating, -2, "Rating must be calculated as average"s);
    }
    ratings = {1, -2, -3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL_HINT(found_docs[0].rating, -1, "Rating must be calculated as average"s);
    }
}

void TestFilterByPredicate() {
    /*
     * Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
     */
    const int doc_id = 48;
    const std::string content = "cat in the city"s;
    const int doc_id1 = 481;
    const std::string content1 = "dog in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
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
        ASSERT_EQUAL(found_docs.size(), 2u);
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
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Incorrect filtering by predicat"s);
    }
}

void TestFindByStatus() {
    /*
     * Поиск документов, имеющих заданный статус.
     */
    const int doc_id = 49;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 1u);
    }
    //
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::BANNED);
        ASSERT_EQUAL_HINT(found_docs.size(), 0u, "Incorrect filtering by status of document"s);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL_HINT(found_docs.size(), 0u, "Incorrect filtering by status of document"s);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL_HINT(found_docs.size(), 0u, "Incorrect filtering by status of document"s);
    }
}

void TestRelevanceCalc() {
    /*
     * Корректное вычисление релевантности найденных документов.
     */
    const int doc_id = 50;
    const std::string content = "cat in the city"s;
    const int doc_id1 = 51;
    const std::string content1 = "dog in the house"s;
    const int doc_id2 = 52;
    const std::string content2 = "horse in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
        auto found_docs = server.FindTopDocuments("city"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 2u);
        ASSERT_HINT(abs(found_docs[0].relevance - 0.1013662770270411) < EPSILON, "Incorrect relevance calculating"s);
        found_docs = server.FindTopDocuments("house"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_HINT(abs(found_docs[0].relevance - 0.27465307216702745) < EPSILON, "Incorrect relevance calculating"s);
        found_docs = server.FindTopDocuments("in"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 3u);
        ASSERT_HINT(abs(found_docs[0].relevance - 0) < EPSILON, "Incorrect relevance calculating"s);
    }

}

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