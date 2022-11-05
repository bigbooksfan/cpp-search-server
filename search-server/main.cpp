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
};

enum class DocumentStatus {     // enum for statuses
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    template <typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query, Predicate predicate) const {
        // predicat - функция. Должна возвращать bool и принимать много всего из лямбды в точке вызова
        const Query query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query, predicate);

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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus stat = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [stat](int document_id, DocumentStatus status, int rating) { return status == stat; });
    }

    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
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
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {                                       // struct < int rating, enum DocStatus status >
        int rating = 0;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;      // map < word, < id , freq > >
    map<int, DocumentData> documents_;                          // map < id, < rating, status >>

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
};

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s << endl;
}

ostream& operator<<(ostream & os, DocumentStatus doc_status) {
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

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

void TestExcludeDocumentsByMinusWords() {
    const int doc_id = 12;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat -city"s);
        ASSERT_EQUAL(found_docs.size(), 0u);
    }    
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("-city"s);
        ASSERT_EQUAL(found_docs.size(), 0u);
    }   
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("-city -city"s);
        ASSERT_EQUAL(found_docs.size(), 0u);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city -city"s);
        ASSERT_EQUAL(found_docs.size(), 0u);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city -a"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
    }
}

void TestMatchingDocumentsByQuerry() {
    const int doc_id = 12;
    const string content = "cat in the city";
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const string query_test = "cat"s;
        tuple<vector<string>, DocumentStatus> expected = server.MatchDocument(query_test, 12);
        vector<string> a;
        DocumentStatus b;
        tie(a, b) = expected;       // tuple decomposing

        ASSERT_EQUAL(a.size(), 1u);
        ASSERT_EQUAL(a[0], "cat"s);
        ASSERT_EQUAL(b, DocumentStatus::ACTUAL);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const string query_test = "cat city"s;
        tuple<vector<string>, DocumentStatus> expected = server.MatchDocument(query_test, 12);

        const vector<string> a = get<0>(expected);
        ASSERT_EQUAL(a.size(), 2u);
        ASSERT_EQUAL(a[0], "cat"s);
        ASSERT_EQUAL(a[1], "city"s);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const string query_test = "cat city -in"s;
        tuple<vector<string>, DocumentStatus> expected = server.MatchDocument(query_test, 12);

        const vector<string> a = get<0>(expected);
        ASSERT_EQUAL(a.size(), 0u);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const string query_test = "cat cat"s;
        tuple<vector<string>, DocumentStatus> expected = server.MatchDocument(query_test, 12);

        const vector<string> a = get<0>(expected);
        ASSERT_EQUAL(a.size(), 1u);
        ASSERT_EQUAL(a[0], "cat"s);
    }
}

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
        SearchServer server;
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
        SearchServer server;
        const vector<int> ratings = { 1, 2, 3 };
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("cat");
        ASSERT_EQUAL(res.size(), 1u);
        int test_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        ASSERT_EQUAL(res[0].rating, test_rating);
    }    
    {
        SearchServer server;            // negative ratings
        const vector<int> ratings = { -1, -2, -4 };
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("cat");
        ASSERT_EQUAL(res.size(), 1u);        
        int test_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        ASSERT_EQUAL(res[0].rating, test_rating);
    }    
    {
        SearchServer server;        // Empty ratings
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
        SearchServer server;
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
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = server.FindTopDocuments("cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        ASSERT_EQUAL(res.size(), 1u);
        ASSERT_EQUAL(res[0].id, 12);
    }   
    {
        SearchServer server;
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
        SearchServer server;
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
        SearchServer server;
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
        ASSERT_HINT(DELTA1 < 1e-6, "Relevance calculation error! Doc relevance ("s + to_string(res[1].relevance) + ") != expected ("s + to_string(test_relevance_doc1) +")");
    }
}

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
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------



int main() {

    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;

#if 0
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s,
        [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
#endif

    return 0;
}
