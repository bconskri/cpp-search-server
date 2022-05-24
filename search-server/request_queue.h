#pragma once

#include <deque>
#include <vector>
#include "document.h"
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer &search_server);

    template<typename DocumentPredicate>
    [[maybe_unused]] std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate);

    [[maybe_unused]] std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentStatus status);

    [[maybe_unused]] std::vector<Document> AddFindRequest(const std::string &raw_query);

    [[maybe_unused]] [[nodiscard]] int GetNoResultRequests() const;

private:
    struct QueryResult {
        uint64_t timestamp;
        int results;
    };

    std::deque<QueryResult> requests_;
    const SearchServer& search_server_;
    int no_results_requests_;
    uint64_t current_time_;
    const static int min_in_day_ = 1440;

    void PutRequestIntoQueue(int results_num);
};

//template function realization
template<typename DocumentPredicate>
[[maybe_unused]] std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate) {
    // напишите реализацию
    const auto matched_documents = search_server_.FindTopDocuments(raw_query, document_predicate);
    RequestQueue::PutRequestIntoQueue(matched_documents.size());
    return matched_documents;
}