//
// Created by Родион Каргаполов on 09.04.2022.
//
#include "remove_duplicates.h"

//out of class procedure
void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> words_of_docs_processed;
    std::set<int> doc_duplicated_ids;

    for (auto doc_found : search_server) {
        const auto& doc_words_freq = search_server.GetWordFrequencies(doc_found);

        std::set<std::string> doc_words_vector;
        for (const auto& [word, _] : doc_words_freq) {
            doc_words_vector.insert(word); // Complexity: Amortized constant.
        } //w*N

        if (!words_of_docs_processed.count(doc_words_vector)) { //Log in the size of the container, O(log(size())).
            words_of_docs_processed.insert(doc_words_vector); // Complexity: Amortized constant
        }
        else {
            doc_duplicated_ids.insert(doc_found); // Complexity: Amortized constant
        }
    }

    for (auto doc_id : doc_duplicated_ids) {
        std::cout << "Found duplicate document id "s << doc_id << std::endl;
        search_server.RemoveDocument(doc_id);
    }
}