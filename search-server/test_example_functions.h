//
// Created by Родион Каргаполов on 24.03.2022.
//
#pragma once

#include <vector>
#include <string>
#include "document.h"
#include "search_server.h"

// ==================== для отладки и примеров =========================
void PrintDocument(const Document& document);

[[maybe_unused]] void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status);

[[maybe_unused]] void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings);
void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query);

[[maybe_unused]] void MatchDocuments(const SearchServer& search_server, const std::string& query);