#include "tests.h"
#include "search_server.h"
#include "request_queue.h"
#include "document.h"
#include "test_example_functions.h"
#include "paginator.h"
#include "remove_duplicates.h"
#include <iostream>
#include "process_queries.h"
#include "../Google_tests/TestProcessQueries.cpp"

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    std::cout << "Search server testing finished"s << std::endl;

    std::cout << std::endl << "---Example of usage 1---"s << std::endl << std::endl;
    SearchServer search_server0("и в на"s);

    AddDocument(search_server0, 1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server0, 1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server0, -1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server0, 3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
    AddDocument(search_server0, 4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 1, 1});

    FindTopDocuments(search_server0, "пушистый -пёс"s);
    FindTopDocuments(search_server0, "пушистый --кот"s);
    FindTopDocuments(search_server0, "пушистый -"s);

    MatchDocuments(search_server0, "пушистый пёс"s);
    MatchDocuments(search_server0, "модный -кот"s);
    MatchDocuments(search_server0, "модный --пёс"s);
    MatchDocuments(search_server0, "пушистый - хвост"s);

    std::cout << std::endl << "---Example of usage 2---"s << std::endl << std::endl;
    SearchServer search_server1("and with"s);

    search_server1.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server1.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server1.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server1.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server1.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});

    //const auto search_results = search_server1.FindTopDocuments("curly dog"s);
    const auto search_results = search_server1.FindTopDocuments("dog cat"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);

    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        std::cout << *page << std::endl;
        std::cout << "Page break"s << std::endl;
    }

    std::cout << std::endl << "---Example of usage 3---"s << std::endl << std::endl;
    SearchServer search_server2("and in at"s);
    RequestQueue request_queue(search_server2);

    search_server2.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server2.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server2.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server2.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server2.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

//    // 1439 запросов с нулевым результатом
//    for (int i = 0; i < 1439; ++i) {
//        request_queue.AddFindRequest("empty request"s);
//    }
//    // все еще 1439 запросов с нулевым результатом
//    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
//    request_queue.AddFindRequest("curly dog"s);
//    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
//    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
//    request_queue.AddFindRequest("big collar"s);
//    // первый запрос удален, 1437 запросов с нулевым результатом
//    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
//    request_queue.AddFindRequest("sparrow"s);
//    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;

    // -- Спринт 5 --
    std::cout << std::endl << "---Sprint 5---"s << std::endl << std::endl;
    SearchServer search_server3("and with"s);

    AddDocument(search_server3, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server3, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // дубликат документа 2, будет удалён
    AddDocument(search_server3, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // отличие только в стоп-словах, считаем дубликатом
    AddDocument(search_server3, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, считаем дубликатом документа 1
    AddDocument(search_server3, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // добавились новые слова, дубликатом не является
    AddDocument(search_server3, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    AddDocument(search_server3, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

    // есть не все слова, не является дубликатом
    AddDocument(search_server3, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

    // слова из разных документов, не является дубликатом
    AddDocument(search_server3, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    std::cout << "Before duplicates removed: "s << search_server3.GetDocumentCount() << std::endl;
    RemoveDuplicates(search_server3);
    std::cout << "After duplicates removed: "s << search_server3.GetDocumentCount() << std::endl;


    return 0;
}