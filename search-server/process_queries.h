//
// Created by Родион Каргаполов on 10.05.2022.
//
#pragma once

#include <vector>
#include <string>
#include <list>
#include "search_server.h"

/*
 *  функция ProcessQueries, распараллеливающую обработку нескольких запросов к поисковой системе.
 *  Она принимает N запросов и возвращает вектор длины N, i-й элемент которого —
 *  результат вызова FindTopDocuments для i-го запроса.
 */
std::vector<std::vector<Document>> ProcessQueries(
        const SearchServer& search_server,
        const std::vector<std::string>& queries);

/*
 * функцию ProcessQueriesJoined. Она должна, подобно функции ProcessQueries,
 * распараллеливать обработку нескольких запросов к поисковой системе,
 * но возвращать набор документов в плоском виде.
 * Функция должна вернуть объект documents.
 * Для него можно написать for (const Document& document : documents)
 * и получить сначала все документы из результата вызова FindTopDocuments для первого запроса,
 * затем для второго и так далее. Количество итераций такого цикла должно быть равно суммарному
 * размеру внутренних векторов, возвращаемых функцией ProcessQueries.
 */
std::list<Document> ProcessQueriesJoined(
        const SearchServer& search_server,
        const std::vector<std::string>& queries);