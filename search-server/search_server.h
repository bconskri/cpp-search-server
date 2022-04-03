#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <map>
#include <set>
#include <tuple>
#include <numeric>
#include <algorithm>
#include <cmath>
#include "document.h"
#include "string_processing.h"
#include "log_duration.h"

using namespace std::string_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

class SearchServer {
public:
    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    int GetDocumentCount() const;
    int GetDocumentId(int index) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;

    bool IsStopWord(const std::string& word) const;

    static bool IsValidWord(const std::string& word);

    const std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string text) const;

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    Query ParseQuery(const std::string& text) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate filter_function) const;
};

//template function realization
template <typename StringContainer>
 SearchServer::SearchServer(const StringContainer& stop_words) {
    if (std::any_of(stop_words.cbegin(), stop_words.cend(), [](const std::string& word){return !SearchServer::IsValidWord(word);}))
    {
        throw std::invalid_argument("Недопустимые символы (с кодами от 0 до 31) в стоп словах"s);
    }
    for (const std::string& word : stop_words) {
        if (word != ""s) {
            stop_words_.insert(word);
        }
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
    //
    LOG_DURATION_STREAM("Operation time", std::cout);
    //
    const SearchServer::Query query = SearchServer::ParseQuery(raw_query);
    auto matched_documents = SearchServer::FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document& lhs, const Document& rhs) {
             if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
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

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const SearchServer::Query& query, DocumentPredicate filter_function) const {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = SearchServer::ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (filter_function(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({
                                            document_id,
                                            relevance,
                                            documents_.at(document_id).rating
                                    });
    }
    return matched_documents;
}