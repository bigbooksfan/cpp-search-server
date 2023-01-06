#include "search_server.h"

#include <algorithm>

std::vector<int>::const_iterator SearchServer::begin()
{
    return ids_.begin();
}

std::vector<int>::const_iterator SearchServer::end()
{
    return ids_.end();
}

const std::map<std::string, double> SearchServer::GetWordFrequencies(int document_id) const
{
    std::map<std::string, double> ret;
    for (const auto word : word_to_document_freqs_) {
        if (word.second.count(document_id)) ret.emplace(word.first, word.second.at(document_id));
    }
    return ret;
}

void SearchServer::RemoveDocument(int document_id)
{

    for (auto word : word_to_document_freqs_) {
        if (word.second.count(document_id)) word.second.erase(document_id);
    }
    if (documents_.count(document_id)) documents_.erase(document_id);
    
    for (std::vector<int>::iterator It = ids_.begin(); It != ids_.end(); ) {
        if (*It == document_id) {
            It = ids_.erase(It);
            //continue;
        }
        else ++It;
    }

    if (words_of_docs_.count(document_id)) words_of_docs_.erase(document_id);

}

void SearchServer::SetStopWords(const std::string& text) {
    for (const std::string& word : SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}

const std::vector<int> SearchServer::CheckDuplicatesInside()
{
    std::vector<int> ret;

    std::map<int, std::set<std::string>>::const_iterator pre_end = words_of_docs_.end();
    if (pre_end != words_of_docs_.begin()) --pre_end;
    else throw std::out_of_range("Something wrong with (words_of_docs_) size!");

    for (std::map<int, std::set<std::string>>::const_iterator It_slow = words_of_docs_.begin();
        It_slow != pre_end; ++It_slow) {

        for (std::map<int, std::set<std::string>>::const_iterator It_fast = It_slow;
            It_fast != pre_end; ) {

            ++It_fast;
            
            if ((*It_slow).second.size() != (*It_fast).second.size()) continue;
            // different number of unique words. Not a twins

            bool flag = true;

            for (std::set<std::string>::const_iterator It_word = (*It_slow).second.begin();
                It_word != (*It_slow).second.end(); ++It_word) {

                flag = true;
                if ((*It_fast).second.count(*It_word) == 0) {
                    flag = false;
                    break;      // slow and fast - not a twins
                }
            }

            if (flag) {
                int twin_id = std::max((*It_slow).first, (*It_fast).first);
                ret.push_back(twin_id);
            }
        }
    }

    return ret;
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {

    if (document_id < 0) throw std::invalid_argument("Negative ID");
    if (documents_.count(document_id)) throw std::invalid_argument("ID already exist");
    if (!IsValidWord(document)) throw std::invalid_argument("Special symbol in AddDocument");

    ids_.push_back(document_id);

    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });

    std::set<std::string> wordset(words.begin(), words.end());
    words_of_docs_.emplace(document_id, wordset);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus stat) const {
    return FindTopDocuments(raw_query, [stat](int document_id, DocumentStatus status, int rating) { return status == stat; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
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
    return std::make_tuple(matched_words, documents_.at(document_id).status);
}

//int SearchServer::GetDocumentId(int index) const {
//    if (index < 0 || index > documents_.size()) {
//        throw std::out_of_range("Wrong document id in GetDocumentId()");
//    }
//    return ids_[index];
//}

SearchServer::SearchServer(const std::string& stop_words) {          // explicit
    if (!IsValidWord(stop_words)) throw std::invalid_argument("Special symbol in constructor (Yandex method)");
    SetStopWords(stop_words);
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {

    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
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

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsValidWord(const std::string& word) {          // static
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

[[nodiscard]] bool SearchServer::CheckDoubleMinus(const std::string& in) const {
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

[[nodiscard]] bool SearchServer::CheckNoMinusWord(const std::string& in) const {
    const unsigned int j = static_cast<unsigned int>(in.size());
    if (in[j - 1] == '-') return false;

    for (unsigned int i = 0; i < j; ++i) {
        if (in[i] == '-') if (in[i + 1] == ' ') return false;
    }

    return true;
}