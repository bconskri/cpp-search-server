//
// Created by Родион Каргаполов on 22.03.2022.
//
#pragma once

#include <iostream>
#include <string>
#include <stdlib.h>
#include "io_functions.h"

using namespace std::string_literals;

#ifndef CPP_SEARCH_SERVER_TESTS_H
#define CPP_SEARCH_SERVER_TESTS_H
#endif //CPP_SEARCH_SERVER_TESTS_H

// -------- Начало модульных тестов поисковой системы ----------
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                     const std::string& func, unsigned line, const std::string& hint);

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
                const std::string& hint);

void TestExcludeStopWordsFromAddedDocumentContent();
void TestAddedDocumentMustBeFind();
void TestMinusWordDocumentsExclude();
void TestDocumentsMatching();
void TestSortByRelevance();
void TestRatingCalculation();
void TestFilterByPredicate();
void TestFindByStatus();
void TestRelevanceCalc();

template <typename T>
void RunTestImpl(T& func, const std::string& name);

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

#define RUN_TEST(func) RunTestImpl((func), #func)

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();

//template function realization
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                     const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cerr << std::boolalpha;
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

template <typename T>
void RunTestImpl(T& func, const std::string& name) {
    func();
    std::cerr << name <<" OK"s << std::endl;
}
// --------- Окончание модульных тестов поисковой системы -----------
