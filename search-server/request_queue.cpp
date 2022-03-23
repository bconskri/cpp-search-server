#include "request_queue.h"

//public:
RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server),
    time_(0) {

}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    auto matched_documents = search_server_.FindTopDocuments(raw_query, status);
    PutRequestIntoQueue(raw_query, matched_documents);
    return matched_documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    auto matched_documents = search_server_.FindTopDocuments(raw_query);
    PutRequestIntoQueue(raw_query, matched_documents);
    return matched_documents;
}

int RequestQueue::GetNoResultRequests() const {
    return count_if(requests_.begin(), requests_.end(), [] (RequestQueue::QueryResult result) {return result.result.empty();});
}


//private:
void RequestQueue::PutRequestIntoQueue(const std::string& raw_query, const std::vector<Document>& result) {
    ++time_;
    if (time_ > min_in_day_) {
        requests_.pop_front();
    }
    requests_.push_back(RequestQueue::QueryResult{time_, raw_query, result});
}
