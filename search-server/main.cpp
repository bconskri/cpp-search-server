#include "tests.h"
#include "search_server.h"
#include "request_queue.h"
#include "document.h"
#include "test_example_functions.h"
#include "paginator.h"

#include <iostream>

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
    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
    request_queue.AddFindRequest("sparrow"s);
    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
    return 0;
}