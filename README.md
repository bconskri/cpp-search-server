# cpp-search-server
Проект: поисковая система

Программа для поиска по ключевым словам в добавленных ранее документах.
Учитывает статус документа и его рейтинг, ранжирует результаты по TF-IDF с учетом стоп слов.
Есть функционал удаления дубликатов документов. Дубликатами считаются документы, у которых наборы встречающихся слов совпадают.
Вывод информации разбивается на страницы.
Для ускорения работы, методы поискового сервера могут обрабатывать запросы как однопоточно, так и в многопоточном варианте.
main.cpp запускает тесты программы и дает представление о вариантах использования программы.


## Setup for CMake

cmake_minimum_required(VERSION 3.21)
project(cpp_search_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE on)

add_subdirectory(Google_tests search-server)

add_executable(cpp-search-server search-server/main.cpp search-server/tests.cpp search-server/string_processing.cpp
search-server/search_server.cpp search-server/search_server.h search-server/request_queue.cpp search-server/read_output_functions.cpp search-server/document.cpp
search-server/paginator.h search-server/test_example_functions.cpp search-server/test_example_functions.h search-server/log_duration.h
search-server/remove_duplicates.cpp search-server/remove_duplicates.h search-server/process_queries.cpp
search-server/process_queries.h Google_tests/test_par_2_3.h search-server/concurrent_map.h)

## Пример использования кода:
```C++
    SearchServer search_server("and with"s);

    for (
        int id = 0;
        const string & text : {
        "white cat and yellow hat"s, "curly cat curly tail"s, "nasty dog with big eyes"s, "nasty pigeon john"s,
        }
    ) 
    {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }
    cout << "ACTUAL by default:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    // параллельная версия
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
```

## Вывод:

ACTUAL by default:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 }
{ document_id = 1, relevance = 0.173287, rating = 2 }
{ document_id = 3, relevance = 0.173287, rating = 1 }
BANNED:
Even ids:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 }
