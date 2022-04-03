#include "search_server.h"

//public:
SearchServer::SearchServer(const std::string& stop_words) : SearchServer::SearchServer(SplitIntoWords(stop_words)) {}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Отрицательный id или id ранее добавленного документа"s);
    }
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{SearchServer::ComputeAverageRating(ratings), status});
    document_ids_.push_back(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(raw_query, [status](int document_id, DocumentStatus lstatus, int rating){
        (void)(document_id);
        (void)(rating);
        return lstatus == status;});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

int SearchServer::GetDocumentId(int index) const {
    if (index >= 0 && index < SearchServer::GetDocumentCount()) {
        return document_ids_[index];
    }
    throw std::out_of_range("Индекс документа за пределами допустимого диапазона (0; количество документов)"s);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    //
    LOG_DURATION_STREAM("Operation time", std::cout);
    //
    const Query query = SearchServer::ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

//private:

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

const std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Недопустимые символы (с кодами от 0 до 31) в тексте добавляемого документа"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    //edited - review recommendation
//    int rating_sum = 0;
//    for (const int rating : ratings) {
//        rating_sum += rating;
//    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    // Empty result by initializing it with default constructed QueryWord
    if (text.empty()) {
        throw std::invalid_argument("ParseQueryWord: Текст запроса некорректен"s);
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !SearchServer::IsValidWord(text)) {
        throw std::invalid_argument("ParseQueryWord: Текст запроса некорректен"s);
    }

    return SearchServer::QueryWord{text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    // Empty result by initializing it with default constructed Query
    SearchServer::Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        const SearchServer::QueryWord query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(SearchServer::GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}