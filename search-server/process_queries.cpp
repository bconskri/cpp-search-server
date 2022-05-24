//
// Created by Родион Каргаполов on 10.05.2022.
//
#include "process_queries.h"
#include <execution>

/*
 *  Функция ProcessQueries, распараллеливающую обработку нескольких запросов к поисковой системе.
 *  Она принимает N запросов и возвращает вектор длины N, i-й элемент которого —
 *  результат вызова FindTopDocuments для i-го запроса.
 */
std::vector<std::vector<Document>> ProcessQueries(
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {

    std::vector<std::vector<Document>> result(queries.size());
    transform(std::execution::par,
              queries.begin(), queries.end(),
              result.begin(),
              [&search_server](const std::string& queries_) { return search_server.FindTopDocuments(queries_); }
    );
    return result;
}

/*
 * Функцию ProcessQueriesJoined. Она должна, подобно функции ProcessQueries,
 * распараллеливать обработку нескольких запросов к поисковой системе,
 * но возвращать набор документов в плоском виде.
 * Функция должна вернуть объект documents.
 * Для него можно написать for (const Document& document : documents)
 * и получить сначала все документы из результата вызова FindTopDocuments для первого запроса,
 * затем для второго и так далее. Количество итераций такого цикла должно быть равно суммарному
 * размеру внутренних векторов, возвращаемых функцией ProcessQueries.
 */
[[maybe_unused]] std::list<Document> ProcessQueriesJoined(
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {
    std::list<Document> doc_list;
    for (const auto& documents : ProcessQueries(search_server, queries)){
        doc_list.insert(doc_list.end(), documents.begin(), documents.end());
    }
    return doc_list;
}