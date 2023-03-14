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
#include <execution>
#include <iostream>

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"
#include "log_duration.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
                 
class SearchServer {

private:                // CLASS INSTANCE FIELDS
    // HINT : map < word, map < id , freq > >
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    // HINT : map < id , map < word , freq > > 
    std::map<int, std::map<std::string_view, double>> words_freqs_overall_;
    std::set<int> ids_;
    std::set<std::string> stop_words_;      // add less<> here

private:                // DOCUMENTS FIELDS
    // HINT : struct < int rating, enum DocStatus status, string content >
    struct DocumentData {
        int rating = 0;
        DocumentStatus status;
        const std::string content;
    };
    // HINT : map < id, struct < rating, status, content >>
    std::map<int, DocumentData> documents_;

private:                // QUERRIES FIELDS
    // HINT : vector <string_view> for execution::par
    struct QueryV {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    // HINT : set <string_view> for execution::seq
    //struct QueryS {
    //    std::set<std::string_view> plus_words;
    //    std::set<std::string_view> minus_words;
    //};

    // HINT : struct < string_view data , bool is_minus , bool is_stop >
    struct QueryWord {
        std::string_view word;
        bool is_minus = true;
        bool is_stop = true;
    };

public:         // constructors

    template<typename T>
    explicit SearchServer(const T& stop_words_container);
    explicit SearchServer(const std::string_view stop_words);
    explicit SearchServer(const std::string stop_words);

public:             // methods

    std::set<int>::const_iterator begin();
    std::set<int>::const_iterator end();

    const std::map<std::string_view, double> GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::parallel_policy& policy, int document_id);
    void RemoveDocument(const std::execution::sequenced_policy& policy, int document_id);

    void AddDocument(int document_id, const std::string_view content, DocumentStatus status, const std::vector<int>& ratings);

    template <typename Predicate, typename Policy>
    std::vector<Document> FindTopDocuments(Policy policy, const std::string_view raw_query, Predicate predicate) const;        // <- the one
    template <typename Policy>
    std::vector<Document> FindTopDocuments(Policy policy, const std::string_view raw_query, DocumentStatus stat) const;        // redirection
    template <typename Policy>
    std::vector<Document> FindTopDocuments(Policy policy, const std::string_view raw_query) const;                             // redirection
    
    template <typename Predicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, Predicate predicate) const;                        // redirection
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus stat) const;                        // redirection
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;                                             // redirection

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> 
        MatchDocument(const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> 
        MatchDocument(const std::execution::parallel_policy& policy, const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> 
        MatchDocument(const std::execution::sequenced_policy& policy, const std::string_view raw_query, int document_id) const;

private:            // methods

    void SetStopWords(const std::string_view text);

    bool IsStopWord(const std::string_view word) const;

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string_view text) const;
    
    //QueryS ParseQueryS(const std::string_view text, bool sort) const;
    QueryV ParseQueryV(const std::string_view text, bool sort = false) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    // Query is QueryS or QueryV
    template <typename Predicate>       // seq
    std::vector<Document> FindAllDocuments(const QueryV& query, Predicate predicate, const std::execution::sequenced_policy& policy = std::execution::seq) const;   
    //std::vector<Document> FindAllDocuments(const QueryS& query, Predicate predicate, const std::execution::sequenced_policy& policy = std::execution::seq) const;   
    template <typename Predicate>       // par
    std::vector<Document> FindAllDocuments(const QueryV& query, Predicate predicate, const std::execution::parallel_policy& policy) const;

    static bool IsValidWord(const std::string_view word);

};

/************************************ TEMPLATE METHODS ************************************/

template<typename T>
SearchServer::SearchServer(const T& stop_words_container) {
    if (stop_words_container.empty()) {
        return;
    }
    
    for (const auto word : stop_words_container) {
        if (!IsValidWord(static_cast<std::string_view>(word))) {
            throw std::invalid_argument("Special symbol in template constructor");
        }
        SetStopWords(word);
    }
}

template<typename Predicate, typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy policy, const std::string_view raw_query, Predicate predicate) const {

    if constexpr (std::is_same_v<Policy, std::execution::sequenced_policy>) {
        const QueryV query = ParseQueryV(raw_query, true);
        //const QueryS query = ParseQueryS(raw_query, true);

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

    QueryV query = ParseQueryV(raw_query, true);
    //std::sort(
    //    std::execution::par,
    //    query1.plus_words.begin(), query1.plus_words.end()
    //);
    //auto last = std::unique(
    //    std::execution::par,
    //    query1.plus_words.begin(), query1.plus_words.end()
    //);
    //query1.plus_words.erase(last, query1.plus_words.end());

    //std::sort(
    //    std::execution::par,
    //    query1.minus_words.begin(), query1.minus_words.end()
    //);
    //last = std::unique(
    //    std::execution::par,
    //    query1.minus_words.begin(), query1.minus_words.end()
    //);
    //query1.minus_words.erase(last, query1.minus_words.end());

    std::vector<Document> matched_documents = FindAllDocuments(query, predicate, std::execution::par);

    std::sort(
        std::execution::par,
        matched_documents.begin(), matched_documents.end(),
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

template <typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy policy, const std::string_view raw_query, DocumentStatus stat) const {
    return FindTopDocuments(policy, raw_query, [stat](int document_id, DocumentStatus status, int rating) { return status == stat; });
}

template <typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy policy, const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
}

template <typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, Predicate predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, predicate);
}

template<typename Predicate>
std::vector<Document> SearchServer::FindAllDocuments(const QueryV& query, Predicate predicate, const std::execution::sequenced_policy& policy) const {
//std::vector<Document> SearchServer::FindAllDocuments(const QueryS& query, Predicate predicate, const std::execution::sequenced_policy& policy) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
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
    for (const std::string_view word : query.minus_words) {
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

template<typename Predicate> 
std::vector<Document> SearchServer::FindAllDocuments(const QueryV& query, Predicate predicate, const std::execution::parallel_policy& policy) const {

    ConcurrentMap<int, double> document_to_relevance(12);
    std::for_each(
        std::execution::par,
        query.plus_words.begin(), query.plus_words.end(),
        [&](const std::string_view word) {
            if (word_to_document_freqs_.count(word) != 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    const DocumentData& a = documents_.at(document_id);
                    if (predicate(document_id, a.status, a.rating)) {
                        document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
        });

    std::map<int, double> assembled_map = document_to_relevance.BuildOrdinaryMap();

    // docs with minus-words removing
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            assembled_map.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : assembled_map) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;

}