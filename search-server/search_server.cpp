#include "search_server.h"

/************************************ CONSTRUCTORS ************************************/

SearchServer::SearchServer(const std::string_view stop_words) {
    if (!IsValidWord(stop_words)) {
        throw std::invalid_argument("Special symbol in constructor");
    }
    SetStopWords(stop_words);
}

SearchServer::SearchServer(const std::string stop_words) {
    if (!IsValidWord(std::string_view(stop_words))) {
        throw std::invalid_argument("Special symbol in constructor");
    }
    SetStopWords(stop_words);
}

/************************************ PRIVATE METHODS ************************************/

void SearchServer::SetStopWords(const std::string_view text) {
    for (const std::string_view& word : SplitIntoWords(text)) {
        stop_words_.insert(std::string(word));
    }
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(std::string(word)) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(text)) {
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
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::QueryS SearchServer::ParseQueryS(const std::string_view text, bool sort) const {
    QueryS query;

    for (const std::string_view& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);

        if (query_word.is_minus && query_word.word.empty()) {
            throw std::invalid_argument("No word after minus in ParseQuery()");
        }
        if (query_word.is_minus && query_word.word[0] == '-') {
            throw std::invalid_argument("Double minus in ParseQuery()");
        }
        if (!IsValidWord(query_word.word)) {
            throw std::invalid_argument("Special symbol in ParseQuery()");
        }

        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.word);              // string_view
            }
            else {
                query.plus_words.insert(query_word.word);              // string_view
            }
        }
    }
    return query;
}

SearchServer::QueryV SearchServer::ParseQueryV(const std::string_view text, bool sort) const {
    QueryV query;

    for (const std::string_view& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);

        if (query_word.is_minus && query_word.word.empty()) {
            throw std::invalid_argument("No word after minus in ParseQuery()");
        }
        if (query_word.is_minus && query_word.word[0] == '-') {
            throw std::invalid_argument("Double minus in ParseQuery()");
        }
        if (!IsValidWord(query_word.word)) {
            throw std::invalid_argument("Special symbol in ParseQuery()");
        }

        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.word);              // string_view
            }
            else {
                query.plus_words.push_back(query_word.word);              // string_view
            }
        }
    }


    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsValidWord(const std::string_view word) {
    return std::none_of(
        word.begin(), word.end(),
        [](const char& c) {
            return c >= '\0' && c < ' ';
        });
}

/************************************ PUBLIC METHODS ************************************/

void SearchServer::AddDocument(int document_id, const std::string_view content, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Negative ID");
    }
    if (documents_.count(document_id)) {
        throw std::invalid_argument("ID already exist");
    }
    if (!IsValidWord(content)) {
        throw std::invalid_argument("Special symbol in AddDocument");
    }

    ids_.insert(document_id);

    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, std::string(content) });

    const std::vector<std::string_view> words = SplitIntoWordsNoStop(std::string_view(documents_.at(document_id).content));
    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        words_freqs_overall_[document_id].emplace(word, word_to_document_freqs_[word][document_id]);
    }
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
    const std::execution::sequenced_policy& policy, const std::string_view raw_query, int document_id) const {
    const QueryS query = ParseQueryS(raw_query, true);
    std::vector<std::string_view> matched_words;

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {     // if no word in document. For .at() safety
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            return std::make_tuple(matched_words, documents_.at(document_id).status);
        }
    }

    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return std::make_tuple(matched_words, documents_.at(document_id).status);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
    const std::execution::parallel_policy& policy, const std::string_view raw_query, int document_id) const {

    if (ids_.count(document_id) == 0)
        throw std::out_of_range("Invalid ID\n");

    QueryV query = ParseQueryV(raw_query, false);      // QueryV - struct of two vectors
    std::vector<std::string_view> matched_words;
    const std::map<std::string_view, double>& InMap = words_freqs_overall_.at(document_id);      // all words of this document

    // return if minus word exists
    if (std::any_of(
        std::execution::par,
        query.minus_words.begin(), query.minus_words.end(),
        [&](const std::string_view word) {
            return InMap.count(word);
        }
    )) {
        //matched_words.clear();
        return std::make_tuple(matched_words, documents_.at(document_id).status);
    }

    // unique words in document
    //matched_words.resize(InMap.size());
    matched_words.resize(query.plus_words.size());

    // filling matched_words
    std::vector<std::string_view>::iterator It = std::copy_if(
        std::execution::par,
        query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),                                          // Implicitly cast string to string_view?
        [&](const std::string_view word) {
            //return InMap.count(word);
            return word_to_document_freqs_.at(word).count(document_id);
        }
    );
    matched_words.erase(It, matched_words.end());

    std::sort(
        std::execution::par,
        matched_words.begin(), matched_words.end()
    );
    auto last = std::unique(
        std::execution::par,
        matched_words.begin(), matched_words.end()
    );
    matched_words.erase(last, matched_words.end());

    return std::make_tuple(matched_words, documents_.at(document_id).status);
}

void SearchServer::RemoveDocument(int document_id) {
    if (ids_.count(document_id) == 0) return;
    SearchServer::RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id) {
    if (ids_.count(document_id) == 0) return;

    for (auto [word, _] : GetWordFrequencies(document_id)) {
        word_to_document_freqs_.at(std::string(word)).erase(document_id);
    }

    if (documents_.count(document_id)) {
        documents_.erase(document_id);
    }

    ids_.erase(document_id);

    words_freqs_overall_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id) {
    if (ids_.count(document_id) == 0) return;

    // vector for words in document to be removed
    const std::map<std::string_view, double>& InMap = words_freqs_overall_.at(document_id);
    std::vector<const std::string_view*> tmp(InMap.size());

    // filling temporary vector
    std::transform(std::execution::par,
        InMap.begin(), InMap.end(),
        tmp.begin(),
        [](const /*std::pair<std::string, double>*/auto& i) {       // WHY std::pair causes UB?
            return &i.first;
        });

    // removing doc_ids for each word
    std::for_each(std::execution::par,
        tmp.begin(), tmp.end(),
        [&](const std::string_view* word) {
            word_to_document_freqs_.at(*word).erase(document_id);
        }
        );

    //  others
    if (documents_.count(document_id)) {
        documents_.erase(document_id);
    }

    ids_.erase(document_id);

    words_freqs_overall_.erase(document_id);
}

const std::map<std::string_view, double> SearchServer::GetWordFrequencies(int document_id) const {
    std::map<std::string_view, double> ret;
    if (words_freqs_overall_.count(document_id) != 0) {
        for (const auto& word : words_freqs_overall_.at(document_id)) {
            ret.emplace(std::string_view(word.first), word.second);
        }
    }
    return ret;
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus stat) const {
    return FindTopDocuments(std::execution::seq, raw_query, [stat](int document_id, DocumentStatus status, int rating) { return status == stat; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
}



/************************************ ITERATORS ************************************/

std::set<int>::const_iterator SearchServer::begin() {
    return ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() {
    return ids_.end();
}