//
// Created by Родион Каргаполов on 23.05.2022.
//
#pragma once

#include "../search-server/search_server.h"
#include "../search-server/process_queries.h"
#include "../search-server/log_duration.h"
#include "../search-server/test_example_functions.h"
#include <iostream>
#include <string>
#include <execution>
#include <random>
#include <vector>

void TestSprint8_Final() {
    using namespace std;

SearchServer search_server("and with"s);

int id = 0;
for (
const string& text : {
"white cat and yellow hat"s,
"curly cat curly tail"s,
"nasty dog with big eyes"s,
"nasty pigeon john"s,
}
) {
search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
}

cout << endl << "Sprint8: Final parallel search"s << endl;
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
for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s,
                                                               [](int document_id, [[maybe_unused]] DocumentStatus status, [[maybe_unused]] int rating) { return document_id % 2 == 0; })) {
PrintDocument(document);
}

}

//string GenerateWord(mt19937& generator, int max_length) {
//    const int length = uniform_int_distribution(1, max_length)(generator);
//    string word;
//    word.reserve(length);
//    for (int i = 0; i < length; ++i) {
//        word.push_back(uniform_int_distribution('a', 'z')(generator));
//    }
//    return word;
//}

//vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
//    vector<string> words;
//    words.reserve(word_count);
//    for (int i = 0; i < word_count; ++i) {
//        words.push_back(GenerateWord(generator, max_length));
//    }
//    words.erase(unique(words.begin(), words.end()), words.end());
//    return words;
//}

//string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
//    string query;
//    for (int i = 0; i < word_count; ++i) {
//        if (!query.empty()) {
//            query.push_back(' ');
//        }
//        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
//            query.push_back('-');
//        }
//        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
//    }
//    return query;
//}

//vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
//    vector<string> queries;
//    queries.reserve(query_count);
//    for (int i = 0; i < query_count; ++i) {
//        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
//    }
//    return queries;
//}

template <typename ExecutionPolicy>
void Test(string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    cout << total_relevance << endl;
}

void Test(const SearchServer& search_server, const vector<string>& queries) {
    LOG_DURATION("No policy");
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(query)) {
            total_relevance += document.relevance;
        }
    }
    cout << total_relevance << endl;
}

#define TEST_NO_POLICY() Test(search_server, queries)

#ifndef TEST
#define TEST(policy) Test(#policy, search_server, queries, execution::policy)
#endif

void TestSprint8_Final_load() {
    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, 1000, 10);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
    }

    const auto queries = GenerateQueries(generator, dictionary, 100, 70);

    TEST_NO_POLICY();
    TEST(seq);
    TEST(par);
}
/*
 * Output:
 * ACTUAL by default:
        { document_id = 2, relevance = 0.866434, rating = 1 }
        { document_id = 4, relevance = 0.231049, rating = 1 }
        { document_id = 1, relevance = 0.173287, rating = 1 }
        { document_id = 3, relevance = 0.173287, rating = 1 }
        BANNED:
        Even ids:
        { document_id = 2, relevance = 0.866434, rating = 1 }
        { document_id = 4, relevance = 0.231049, rating = 1 }
 */