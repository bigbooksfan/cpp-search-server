#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <set>
#include <map>
#include <numeric>
#include <algorithm>
#include <cmath>

#include "document.h"
#include "string_processing.h"
#include "log_duration.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {

private:            // fields
    // HINT : struct < int rating, enum DocStatus status >
    struct DocumentData {                                       
        int rating = 0;
        DocumentStatus status;
    };
    
    struct QueryWord {
        std::string data;
        bool is_minus = true;
        bool is_stop = true;
    };

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    std::set<std::string> stop_words_;

    // HINT : map < word, map < id , freq > >
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;

    // HINT : map < id, < rating, status >>
    std::map<int, DocumentData> documents_;                          

    std::vector<int> ids_;
    // Do i need it?
    // std::map<int, std::set<std::string>> words_of_docs_;

public:

    // HINT : map < id , map < word , freq > > 
    static std::map<int, std::map<std::string, double>> words_freqs_overall_;

public:      // methods

    std::vector<int>::const_iterator begin();
    std::vector<int>::const_iterator end();

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;
 // const - for return type                                                  const - for calling on const object

    void RemoveDocument(int document_id);

    void SetStopWords(const std::string& text);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename Predicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, Predicate predicate) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus stat) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

public:         // constructors

    explicit SearchServer(const std::string& stop_words);

    template<typename T>
    explicit SearchServer(const T& stop_words_container);

private:            // methods

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string text) const;

    Query ParseQuery(const std::string& text) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename Predicate>
    std::vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const;
   
    static bool IsValidWord(const std::string& word);

};

template <typename Predicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, Predicate predicate) const {

    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const DocumentData& a = documents_.at(document_id);
            if (predicate(document_id, a.status, a.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    // docs with minus-words removing
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
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template<typename T>
SearchServer::SearchServer(const T& stop_words_container) {
    if (stop_words_container.empty()) {
        return;
    }

    for (std::string word : stop_words_container) {
        if (IsValidWord(word)) {
            stop_words_.insert(word);
        }
        else {
            throw std::invalid_argument("Special symbol in template constructor");
        }
    }
}

template <typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, Predicate predicate) const {

    const Query query = ParseQuery(raw_query);

    std::vector<Document> matched_documents = FindAllDocuments(query, predicate);

    std::sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            const double DELTA = 1e-6;
    if (std::abs(lhs.relevance - rhs.relevance) < DELTA) {
        return lhs.rating > rhs.rating;
    }
    return lhs.relevance > rhs.relevance;
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}