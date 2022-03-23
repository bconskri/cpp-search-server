#pragma once
#include <deque>
#include <vector>
#include "document.h"
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
    
private:
    struct QueryResult {
        int timestamp;
        const std::string& raw_query;
        const std::vector<Document> result;
    };
    
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int time_;
    
    void PutRequestIntoQueue(const std::string& raw_query, const std::vector<Document>& result);
};

//tempate function realization
template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        // напишите реализацию
        auto matched_documents = search_server_.FindTopDocuments(raw_query, document_predicate);
    RequestQueue::PutRequestIntoQueue(raw_query, matched_documents);
        return matched_documents;
    }
