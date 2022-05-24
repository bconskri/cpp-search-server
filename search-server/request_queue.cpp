#include "request_queue.h"

//public:
RequestQueue::RequestQueue(const SearchServer& search_server)
        : search_server_(search_server)
        , no_results_requests_(0)
        , current_time_(0) {

}

[[maybe_unused]] std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    const auto matched_documents = search_server_.FindTopDocuments(raw_query, status);
    PutRequestIntoQueue(matched_documents.size());
    return matched_documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    const auto matched_documents = search_server_.FindTopDocuments(raw_query);
    PutRequestIntoQueue(matched_documents.size());
    return matched_documents;
}

[[maybe_unused]] int RequestQueue::GetNoResultRequests() const {
    return no_results_requests_;
}


//private:
void RequestQueue::PutRequestIntoQueue(int results_num) {
    // новый запрос - новая секунда
    ++current_time_;
    // удаляем все результаты поиска, которые устарели
    while (!requests_.empty() && min_in_day_ <= current_time_ - requests_.front().timestamp) {
        if (0 == requests_.front().results) {
            --no_results_requests_;
        }
        requests_.pop_front();
    }
    // сохраняем новый результат поиска
    requests_.push_back({current_time_, results_num});
    if (0 == results_num) {
        ++no_results_requests_;
    }
}