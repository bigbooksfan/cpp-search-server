#include "search_server.h"

void SearchServer::SetStopWords(const string& text) {
    for (const string& word : SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status, const std::vector<int>& ratings) {

    if (document_id < 0) throw std::invalid_argument("Negative ID");
    if (documents_.count(document_id)) throw std::invalid_argument("ID already exist");
    if (!IsValidWord(document)) throw std::invalid_argument("Special symbol in AddDocument");

    ids_.push_back(document_id);

    const std::vector<string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
}

std::vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus stat) const {
    return FindTopDocuments(raw_query, [stat](int document_id, DocumentStatus status, int rating) { return status == stat; });
}

std::vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

std::tuple<std::vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
    std::vector<string> matched_words;
    for (const string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return make_tuple(matched_words, documents_.at(document_id).status);
}

int SearchServer::GetDocumentId(int index) const {
    if (index < 0 || index > documents_.size()) {
        throw std::out_of_range("Wrong document id in GetDocumentId()");
    }
    return ids_[index];
}

SearchServer::SearchServer(const string& stop_words) {          // explicit
    if (!IsValidWord(stop_words)) throw std::invalid_argument("Special symbol in constructor (Yandex method)");
    SetStopWords(stop_words);
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    std::vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {  // static! check header
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {

    Query query;
    for (const string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);

        if (query_word.is_minus && query_word.data[0] == '-') throw std::invalid_argument("Double minus in ParseQuery()");
        if (!IsValidWord(query_word.data)) throw std::invalid_argument("Special symbol in ParseQuery()");
        if (query_word.is_minus && query_word.data.empty()) throw std::invalid_argument("No word after minus in ParseQuery()");


        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            }
            else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsValidWord(const string& word) {          // static
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

[[nodiscard]] bool SearchServer::CheckDoubleMinus(const string& in) const {
    bool prev_minus = false;
    for (const char letter : in) {
        if (letter == '-') {
            if (prev_minus == true) {
                return false;
            }
            else prev_minus = true;
        }
        else prev_minus = false;
    }

    return true;
}

[[nodiscard]] bool SearchServer::CheckNoMinusWord(const string& in) const {
    const unsigned int j = static_cast<unsigned int>(in.size());
    if (in[j - 1] == '-') return false;

    for (unsigned int i = 0; i < j; ++i) {
        if (in[i] == '-') if (in[i + 1] == ' ') return false;
    }

    return true;
}