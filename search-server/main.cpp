#if 1

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <cassert>
#include <optional>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id = 0;
    double relevance = 0.0;
    int rating = 0;

    Document() : id(0), relevance(0.0), rating(0) {}

    Document(int id_in, double relevance_in, int rating_in) : id(id_in), relevance(relevance_in), rating(rating_in) {}
};

enum class DocumentStatus {     // enum for statuses
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {

        if (document_id < 0) throw invalid_argument("Negative ID"s);
        if (documents_.count(document_id)) throw invalid_argument("ID already exist"s);
        if (!CheckSpecialSymbols(document)) throw invalid_argument("Special symbol in AddDocument (my method)"s);
        if (!IsValidWord(document)) throw invalid_argument("Special symbol in AddDocument (Yandex method)"s);

        ids_.push_back(document_id);

        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        //return true;
    }
    
    template <typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query, Predicate predicate) const {
        // predicat - функтор. Должен возвращать bool и принимать много всего из лямбды в точке вызова
        if (!CheckDoubleMinus(raw_query)) throw invalid_argument("Double minus in FindTopDocument()"s);
        if (!CheckNoMinusWord(raw_query)) throw invalid_argument("No word after minus in FindTopDocument()"s);
        if (!CheckSpecialSymbols(raw_query)) throw invalid_argument("Special symbol in FindTopDocuments() (my method)"s);
        if (!IsValidWord(raw_query)) throw invalid_argument("Special symbol in FindTopDocuments() (Yandex method)"s);

        const Query query = ParseQuery(raw_query);

        vector<Document> matched_documents = FindAllDocuments(query, predicate);

        sort(matched_documents.begin(), matched_documents.end(),
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus stat) const {
        return FindTopDocuments(raw_query, [stat](int document_id, DocumentStatus status, int rating) { return status == stat; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
        //return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }


    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        if (!CheckDoubleMinus(raw_query)) throw invalid_argument("Double minus in MatchDocument()"s);
        if (!CheckNoMinusWord(raw_query)) throw invalid_argument("No word after minus in MatchDocument()"s);
        if (!CheckSpecialSymbols(raw_query)) throw invalid_argument("Special symbol in MatchDocuments() (my method)"s);
        if (!IsValidWord(raw_query)) throw invalid_argument("Special symbol in MatchDocuments() (Yandex method)"s);

        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
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

    int GetDocumentId(int index) const {
        if (index < 0 || index > documents_.size()) {
            throw out_of_range("Wrong document id in GetDocumentId()"s);
        }

        //map<int, DocumentData>::const_iterator it = documents_.cbegin();      // След от неоднозначно изложенного задания
        //for (; index > 0; --index) ++it;
        //return (*it).first;
            return ids_[index];
        
    }

public:         // constructors

    explicit SearchServer(const string& stop_words) {
        if (!CheckSpecialSymbols(stop_words)) throw invalid_argument("Special symbol in constructor (my method)"s);
        if (!IsValidWord(stop_words)) throw invalid_argument("Special symbol in constructor (Yandex method)"s);
        SetStopWords(stop_words);
    }

    template<typename T>
    explicit SearchServer(const T& stop_words_container) {
        if (stop_words_container.empty()) return;

        for (string word : stop_words_container) {
            if (CheckSpecialSymbols(word) && IsValidWord(word)) stop_words_.insert(word);
            else throw invalid_argument("Special symbol in template constructor"s);
        }

    }


private:
    struct DocumentData {                                       // struct < int rating, enum DocStatus status >
        int rating = 0;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;      // map < word, < id , freq > >
    map<int, DocumentData> documents_;                          // map < id, < rating, status >>

    vector<int> ids_;               // КОСТЫЛЬ!!!!!! Я не хотел, меня заставили 

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus = true;
        bool is_stop = true;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
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

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Predicate>
    vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const {                // predicate from lambda rethrowed here

        map<int, double> document_to_relevance;
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

        vector<Document> matched_documents;                                                         // finalists
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }

    [[nodiscard]] bool CheckSpecialSymbols(const string& in) const {

        unsigned int let_num = 0;
        for (const char letter : in) {
            let_num = static_cast<unsigned int>(letter);
            //if (let_num < 32 || (let_num > 32 && let_num < 65) || (let_num > 90 && let_num < 97) || (let_num > 122 && let_num < 4294967232)) return false;
            if (let_num < 32) return false;
        }

        return true;
    }

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    [[nodiscard]] bool CheckDoubleMinus(const string& in) const {
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

    [[nodiscard]] bool CheckNoMinusWord(const string& in) const {
        const unsigned int j = static_cast<unsigned int>(in.size());
        if (in[j - 1] == '-') return false;

        for (unsigned int i = 0; i < j; ++i) {
            if (in[i] == '-') if (in[i + 1] == ' ') return false;
        }

        return true;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s << endl;
}

ostream& operator<<(ostream& os, DocumentStatus doc_status) {
    if (doc_status == DocumentStatus::ACTUAL) os << "actual"s;
    if (doc_status == DocumentStatus::BANNED) os << "banned"s;
    if (doc_status == DocumentStatus::IRRELEVANT) os << "irrelevant"s;
    if (doc_status == DocumentStatus::REMOVED) os << "removed"s;
    return os;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file, const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

template <typename T>
void RunTestImpl(const T& func, const string& func_name) {
    func();
    cerr << func_name << " OK\n";
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

#define RUN_TEST(func) RunTestImpl((func), #func)


// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов

#if 1
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(res.size(), 1u);
        const Document& doc0 = res[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("in"s);
        ASSERT_HINT(res.empty(), "Stop words must be excluded from documents"s);
    }
}

#endif
#if 1

void TestExcludeDocumentsByMinusWords() {
    const int doc_id = 12;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("cat -city"s);
        ASSERT_EQUAL(res.size(), 0u);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("-city"s);
        ASSERT_EQUAL(res.size(), 0u);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("-city -city"s);
        ASSERT_EQUAL(res.size(), 0u);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("city -city"s);
        ASSERT_EQUAL(res.size(), 0u);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("city -a"s);
        ASSERT_EQUAL(res.size(), 1u);
    }
}

#endif
#if 1

void TestMatchingDocumentsByQuerry() {
    const int doc_id = 12;
    const string content = "cat in the city";
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const string query_test = "cat"s;
        tuple<vector<string>, DocumentStatus> expected = server.MatchDocument(query_test, 12);

        auto& [a, b] = expected;

        //vector<string> a;
        //DocumentStatus b;
        //tie(a, b) = expected;       // tuple decomposing

        ASSERT_EQUAL(a.size(), 1u);
        ASSERT_EQUAL(a[0], "cat"s);
        ASSERT_EQUAL(b, DocumentStatus::ACTUAL);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const string query_test = "cat city"s;
        tuple<vector<string>, DocumentStatus> expected = server.MatchDocument(query_test, 12);

        const vector<string> a = get<0>(expected);
        ASSERT_EQUAL(a.size(), 2u);
        ASSERT_EQUAL(a[0], "cat"s);
        ASSERT_EQUAL(a[1], "city"s);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const string query_test = "cat city -in"s;
        tuple<vector<string>, DocumentStatus> expected = server.MatchDocument(query_test, 12);

        const vector<string>& a = get<0>(expected);
        ASSERT_EQUAL(a.size(), 2u);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const string query_test = "cat cat"s;
        tuple<vector<string>, DocumentStatus> expected = server.MatchDocument(query_test, 12);

        const vector<string> a = get<0>(expected);
        ASSERT_EQUAL(a.size(), 1u);
        ASSERT_EQUAL(a[0], "cat"s);
    }

    try {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        tuple<vector<string>, DocumentStatus> expected = server.MatchDocument("cat --cat"s, 12);
        ASSERT_HINT(false, "Check MatchDocument() and CheckDoubleMinus()");
    }
    catch (const invalid_argument& e) {
        // Do nothing
    }
    catch (...) {
        ASSERT_HINT(false, "Check MatchDocument() and CheckDoubleMinus()! Wrong exception has been throwed");
    }

    try {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        tuple<vector<string>, DocumentStatus> expected = server.MatchDocument("cat -"s, 12);
        ASSERT_HINT(false, "Check MatchDocument() and CheckNoMinusWord()");
    }
    catch (const invalid_argument& e) {
        // Do nothing
    }
    catch (...) {
        ASSERT_HINT(false, "Check MatchDocument() and CheckNoMinusWord()! Wrong exception has been throwed");
    }

    try {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, "city-cat out in", DocumentStatus::ACTUAL, ratings);
        const string query_test = "city-cat"s;
    }
    catch (...) {
        ASSERT_HINT(false, "Check MatchDocument() and CheckNoMinusWord()! Minus in middle of the word causes exception");
    }
}

#endif
#if 1

void TestSortResultsByRelevance() {
    const int doc_id1 = 12;
    const string content1 = "cat in the city";
    const vector<int> ratings1 = { 1, 2, 3 };
    const int doc_id2 = 13;
    const string content2 = "dog out woods city";
    const vector<int> ratings2 = { 1, 3, 5 };
    const int doc_id3 = 14;
    const string content3 = "cat the city";
    const vector<int> ratings3 = { 2, 3, 4 };
    const int doc_id4 = 15;
    const string content4 = "wolf sea boat";
    const vector<int> ratings4 = { 6, 7, 8 };

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);

        vector<Document> result = server.FindTopDocuments("city cat"s);
        ASSERT_EQUAL(result.size(), 3u);
        ASSERT_EQUAL(result[0].id, 14);
        ASSERT_EQUAL(result[1].id, 12);
        ASSERT_EQUAL(result[2].id, 13);
        ASSERT(result[0].relevance >= result[1].relevance && result[1].relevance >= result[2].relevance);
        //double rel1 = 0.32694308433724206;
        //ASSERT_EQUAL(result[0].relevance, rel1);
        //double rel2 = 0.24520731325293155;
        //ASSERT_EQUAL(result[1].relevance, rel2);
        //double rel3 = 0.071920518112945211;
        //ASSERT_EQUAL(result[2].relevance, rel3);              // too much. Here just sort checking. Relevance calculation check in corresponding test hereinafter
    }
}

void TestAverageRatingCalculation() {
    const int doc_id = 12;
    const string content = "cat in the city";

    {
        SearchServer server("in the"s);
        const vector<int> ratings = { 1, 2, 3 };
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("cat");
        ASSERT_EQUAL(res.size(), 1u);
        int test_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        ASSERT_EQUAL(res[0].rating, test_rating);
    }
    {
        SearchServer server("in the"s);            // negative ratings
        const vector<int> ratings = { -1, -2, -4 };
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("cat");
        ASSERT_EQUAL(res.size(), 1u);
        int test_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        ASSERT_EQUAL(res[0].rating, test_rating);
    }
    {
        SearchServer server("in the"s);        // Empty ratings
        vector<int> ratings;
        ratings.clear();
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("cat");
        ASSERT_EQUAL(res.size(), 1u);
        ASSERT_EQUAL(res[0].rating, 0);
    }

    const int doc_id1 = 13;
    const string content1 = "dog out woods city";

    {
        SearchServer server("in the"s);
        const vector<int> ratings = { 0, 1, 2 };
        const vector<int> ratings1 = { 3, 4, 5 };
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        vector<Document> res = server.FindTopDocuments("city cat");
        ASSERT_EQUAL(res.size(), 2u);
        ASSERT_EQUAL(res[0].rating, 1);
        ASSERT_EQUAL(res[1].rating, 4);
    }
}

void TestFindingDocumentByPredicate() {
    const int doc_id = 12, doc_id1 = 13;
    const string content = "cat in the city", content1 = "dog out woods city";
    vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        ASSERT_EQUAL(res.size(), 1u);
        ASSERT_EQUAL(res[0].id, 12);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("city"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
        ASSERT_EQUAL(res.size(), 2u);
        ASSERT_EQUAL(res[0].id, 12);
        ASSERT_EQUAL(res[1].id, 13);
    }
}

void TestFindingDocumentByStatus() {
    const int doc_id = 12, doc_id1 = 13;
    const string content = "cat in the city", content1 = "dog out woods city";
    vector<int> ratings = { 1, 2, 3 };
   {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::IRRELEVANT, ratings);
        vector<Document> res = server.FindTopDocuments("city"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(res.size(), 1u);
        ASSERT_EQUAL(res[0].id, 12);
    }
}

void TestRelevanceCalculation() {
    const int doc_id = 12, doc_id1 = 13;
    const string content = "cat in the city", content1 = "dog out woods city";
    vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server("AAA BBB   ccc  "s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("cat city"s);
        ASSERT_EQUAL(res.size(), 2u);

        double test_idf_cat = log(2.0 / 1.0); // ( doc count / docs with word "cat" )
        double test_idf_city = log(2.0 / 2.0); // ( doc count / docs with word "city" )      log(1) == 0
        double test_tf_cat_doc0 = 1.0 / 4.0;     // ( word "cat" count / number of words ) in doc 0
        double test_tf_cat_doc1 = 0.0 / 4.0;     // ( word "cat" count / number of words ) in doc 1
        double test_tf_city_doc0 = 1.0 / 4.0;     // ( word "city" count / number of words ) in doc 0
        double test_tf_city_doc1 = 1.0 / 4.0;     // ( word "city" count / number of words ) in doc 1

        double test_relevance_doc0 = test_idf_cat * test_tf_cat_doc0 + test_idf_city * test_tf_city_doc0;        // 0.17328679513998632
        double DELTA0 = abs((res[0].relevance - test_relevance_doc0));
        ASSERT_HINT(DELTA0 < 1e-6, "Relevance calculation error! Doc relevance ("s + to_string(res[1].relevance) + ") != expected ("s + to_string(test_relevance_doc0) + ")");

        double test_relevance_doc1 = test_idf_cat * test_tf_cat_doc1 + test_idf_city * test_tf_city_doc1;        // 0
        double DELTA1 = abs((res[1].relevance - test_relevance_doc1));
        ASSERT_HINT(DELTA1 < 1e-6, "Relevance calculation error! Doc relevance ("s + to_string(res[1].relevance) + ") != expected ("s + to_string(test_relevance_doc1) + ")");
    }
}

void TestServerConstructingByContqainers() {
    const string content = "cat in the city"s;
    const string content1 = "dog out woods city"s;

    {
        const vector<string> test_stop_words = { "in", "the" };
        SearchServer server(test_stop_words);
        server.AddDocument(12, content, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(13, content1, DocumentStatus::ACTUAL, { 2, 3, 4 });
        vector<Document> res = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL_HINT(res.size(), 1u, "Server construction via vector<string> fails"s);
        vector<Document> res1 = server.FindTopDocuments("city"s);
        ASSERT_EQUAL_HINT(res1.size(), 2u, "Server construction via vector<string> fails"s);
    }
    {
        const set<string> test_stop_words = { "in", "the" };
        SearchServer server(test_stop_words);
        server.AddDocument(12, content, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(13, content1, DocumentStatus::ACTUAL, { 2, 3, 4 });
        vector<Document> res = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL_HINT(res.size(), 1u, "Server construction via vector<string> fails"s);
        vector<Document> res1 = server.FindTopDocuments("city"s);
        ASSERT_EQUAL_HINT(res1.size(), 2u, "Server construction via vector<string> fails"s);
    }
    {
        const vector<string> test_stop_words = {};
        SearchServer server(test_stop_words);
        server.AddDocument(12, content, DocumentStatus::ACTUAL, { 1, 2, 3 });
        vector<Document> res = server.FindTopDocuments("in"s);
        ASSERT_EQUAL_HINT(res.size(), 1u, "Check server constructing with empty stop-words vector<string>"s);
    }

}

#endif
#if 1

void TestAddDocuments() {
    const string content = "cat in the city"s;
    const vector<string> test_stop_words = { "in"s, "the"s };

    try {
        SearchServer server(test_stop_words);
        server.AddDocument(12, content, DocumentStatus::ACTUAL, { 1, 2, 3 });
    }
    catch (...) {
        ASSERT_HINT(false, "Check bool AddDocument()! Valid condition causes exception");
    }

    try {
        SearchServer server(test_stop_words);
        server.AddDocument(-1, content, DocumentStatus::ACTUAL, { 1, 2, 3 });
        ASSERT_HINT(false, "Check bool AddDocument()! Negative ID don't causes throwing invalid_argument"s);
    }
    catch (const invalid_argument& e) {
        // Everything is OK
    }
    catch (...) {
        ASSERT_HINT(false, "Check bool AddDocument()! Negative ID, wrong exception has been throwed");
    }

    try {
        SearchServer server(test_stop_words);
        server.AddDocument(0, content, DocumentStatus::ACTUAL, { 1, 2, 3 });
    }
    catch (...) {
        ASSERT_HINT(false, "Check bool AddDocument()! ID=0 has throwed exception");
    }

    try {
        SearchServer server(test_stop_words);
        server.AddDocument(12, content, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(12, "dog out of woods"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        ASSERT_HINT(false, "Check bool AddDocument()! Double ID, exception hasn't been throwed"s);
    }
    catch (const invalid_argument& e) {
        // Do nothing
    }
    catch (...) {
        ASSERT_HINT(false, "Check bool AddDocument()! Double ID, wrong exception has been throwed");
    }

    try {
        SearchServer server(test_stop_words);
        server.AddDocument(12, "cat in the c\x12ity"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        ASSERT_HINT(false, "Check bool AddDocument()! Special symbols, exception hasn't been throwed"s);
    }
    catch (const invalid_argument& e) {
        // Everything is OK
    }
    catch (...) {
        ASSERT_HINT(false, " Check AddDocument()! Special symbols in querry causes wrong exception"s);
    }

    try {
        SearchServer server(test_stop_words);
        server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    }
    catch (...) {
        ASSERT_HINT(false, "Check bool AddDocument()! Russian letters has throwed exception");
    }

    try {
        SearchServer server(test_stop_words);
        server.AddDocument(12, "белый кот и мо\x12дный ошейник"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        ASSERT_HINT(false, "Check bool AddDocument()! Special symbols in russian querry, exception hasn't been throwed"s);
    }
    catch (const invalid_argument& e) {
        // Do nothing
    }
    catch (...) {
        ASSERT_HINT(false, "Check bool AddDocument()! Special symbols in russian querry, wrong exception has been throwed");
    }

}

#endif
#if 1

void TestGetDocIDByNumber() {
    const string content0 = "aaa";
    const string content1 = "bbb";
    const string content2 = "ccc";
    const string content3 = "ddd";
    const string content4 = "eee";
    const vector<string> test_stop_words = { "in"s, "the"s };

    SearchServer server(test_stop_words);
    server.AddDocument(12, content0, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(13, content1, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(14, content2, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(15, content3, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(16, content4, DocumentStatus::ACTUAL, { 1, 2, 3 });
    {
        int get_id = server.GetDocumentId(0);
        ASSERT_EQUAL_HINT(get_id, 12, "Check GetDocumentId()! Number 0 getted wrong");
    }
    {
        int get_id = server.GetDocumentId(1);
        ASSERT_EQUAL_HINT(get_id, 13, "Check GetDocumentId()! Number 1 getted wrong");
    }
    {
        int get_id = server.GetDocumentId(4);
        ASSERT_EQUAL_HINT(get_id, 16, "Check GetDocumentId()! Number 4 (last) getted wrong");
    }

    try {
        server.GetDocumentId(10);
        ASSERT_HINT(false, "Check GetDocumentId()! Out of range check might be missed"s);
    }
    catch (const out_of_range& e) {
        // Do nothing
    }
    catch (...) {
        ASSERT_HINT(false, "Check GetDocumentId()! Out of range ID, wrong exception has been throwed");
    }
    
    try {
        server.GetDocumentId(-6);
        ASSERT_HINT(false, "Check GetDocumentId()! Negative doc number check might be missed"s);
    }
    catch (const out_of_range& e) {
        // Do nothing
    }
    catch (...) {
        ASSERT_HINT(false, "Check GetDocumentId()! Negative doc number, wrong exception has been throwed");
    }
}

#endif
#if 1

/*
Разместите код остальных тестов здесь
*/

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeDocumentsByMinusWords);
    RUN_TEST(TestMatchingDocumentsByQuerry);
    RUN_TEST(TestSortResultsByRelevance);
    RUN_TEST(TestAverageRatingCalculation);
    RUN_TEST(TestFindingDocumentByPredicate);
    RUN_TEST(TestFindingDocumentByStatus);
    RUN_TEST(TestRelevanceCalculation);
    RUN_TEST(TestServerConstructingByContqainers);
    RUN_TEST(TestAddDocuments);
    RUN_TEST(TestGetDocIDByNumber);
    // Не забудьте вызывать остальные тесты здесь
}
#endif
// --------- Окончание модульных тестов поисковой системы -----------

int main() {

    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cerr << "Search server testing finished"s << endl;

#if 0
    SearchServer search_server("и в на"s);
    // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
    // о неиспользуемом результате его вызова
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    if (!search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 })) {
        cout << "Документ не был добавлен, так как его id совпадает с уже имеющимся"s << endl;
    }
    if (!search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 })) {
        cout << "Документ не был добавлен, так как его id отрицательный"s << endl;
    }
    if (!search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 3, 2 })) {
        cout << "Документ не был добавлен, так как содержит спецсимволы"s << endl;
    }
    vector<Document> documents;
    if (search_server.FindTopDocuments("--пушистый"s, documents)) {
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }
    else {
        cout << "Ошибка в поисковом запросе"s << endl;
    }

#endif


    return 0;
}
#endif
