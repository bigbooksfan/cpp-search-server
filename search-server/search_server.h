#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <set>
#include <map>
#include <numeric>
#include <algorithm>

#include "document.h"
#include "string_processing.h"

using std::string;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    void SetStopWords(const string& text);

    void AddDocument(int document_id, const string& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename Predicate>
    std::vector<Document> FindTopDocuments(const string& raw_query, Predicate predicate) const {
        // predicate - функтор. Должен возвращать bool и принимать много всего из лямбды в точке вызова

        const Query query = ParseQuery(raw_query);

        std::vector<Document> matched_documents = FindAllDocuments(query, predicate);

        std::sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                    return lhs.rating > rhs.rating;
                }
        return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    std::vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus stat) const;

    std::vector<Document> FindTopDocuments(const string& raw_query) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const;

    int GetDocumentId(int index) const;

public:         // constructors

    explicit SearchServer(const string& stop_words);

    template<typename T>
    explicit SearchServer(const T& stop_words_container) {
        if (stop_words_container.empty()) return;

        for (string word : stop_words_container) {
            if (/*CheckSpecialSymbols(word) && */IsValidWord(word)) stop_words_.insert(word);
            else throw std::invalid_argument("Special symbol in template constructor");
        }
    }

private:
    struct DocumentData {                                       // struct < int rating, enum DocStatus status >
        int rating = 0;
        DocumentStatus status;
    };

    std::set<string> stop_words_;
    std::map<string, std::map<int, double>> word_to_document_freqs_;      // map < word, < id , freq > >
    std::map<int, DocumentData> documents_;                          // map < id, < rating, status >>

    std::vector<int> ids_;               // КОСТЫЛЬ!!!!!! Я не хотел, меня заставили

    bool IsStopWord(const string& word) const;

    std::vector<string> SplitIntoWordsNoStop(const string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        string data;
        bool is_minus = true;
        bool is_stop = true;
    };

    QueryWord ParseQueryWord(string text) const;

    struct Query {
        std::set<string> plus_words;
        std::set<string> minus_words;
    };

    Query ParseQuery(const string& text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const;

    template <typename Predicate>
    std::vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const {                // predicate from lambda rethrowed here

        std::map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);              // IDF calculation
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const DocumentData& a = documents_.at(document_id);                                 // temporary object
                if (predicate(document_id, a.status, a.rating)) {                                    // lambda uses HERE
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {                                              // docs with minus-words removing
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> matched_documents;                                                         // finalists
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }

    static bool IsValidWord(const string& word);

    [[nodiscard]] bool CheckDoubleMinus(const string& in) const;

    [[nodiscard]] bool CheckNoMinusWord(const string& in) const;
};
