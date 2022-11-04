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

    template <typename Predicat>
    vector<Document> FindTopDocuments(const string& raw_query, Predicat predicat) const {
        // predicat - функция. Должна возвращать bool и принимать много всего из лямбды в точке вызова
        const Query query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query, predicat);

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

    template <typename Predicat>
    vector<Document> FindAllDocuments(const Query& query, Predicat predicat) const {                // predicat from lambda rethrowed here

        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);              // IDF calculation
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const DocumentData& a = documents_.at(document_id);                                 // temporary object
                if (predicat(document_id, a.status, a.rating)) {                                    // lambda uses HERE
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

ostream& operator<< (ostream & os, DocumentStatus doc_status) {
    if (doc_status == DocumentStatus::ACTUAL) os << DocumentStatus::ACTUAL;
    if (doc_status == DocumentStatus::BANNED) os << DocumentStatus::BANNED;
    if (doc_status == DocumentStatus::IRRELEVANT) os << DocumentStatus::IRRELEVANT;
    if (doc_status == DocumentStatus::REMOVED) os << DocumentStatus::REMOVED;
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

void TestMinusWords() {
    const int doc_id = 12;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = srv.FindTopDocuments("cat -city"s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }    
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = srv.FindTopDocuments("-city"s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }   
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = srv.FindTopDocuments("-city -city"s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = srv.FindTopDocuments("city -city"s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = srv.FindTopDocuments("city -a"s);
        ASSERT_EQUAL(found_docs.size(), 1);
    }
}

void TestMatching() {
    const int doc_id = 12;
    const string content = "cat in the city";
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const string query_test = "cat"s;
        tuple<vector<string>, DocumentStatus> expected = srv.MatchDocument(query_test, 12);

        const vector<string> a = get<0>(expected);
        const DocumentStatus b = get<1>(expected);
        ASSERT_EQUAL(a[0], "cat"s);
        ASSERT_EQUAL(b, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(a.size(), 1);
    }
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const string query_test = "cat city"s;
        tuple<vector<string>, DocumentStatus> expected = srv.MatchDocument(query_test, 12);

        const vector<string> a = get<0>(expected);
        ASSERT_EQUAL(a[0], "cat"s);
        ASSERT_EQUAL(a[1], "city"s);
        ASSERT_EQUAL(a.size(), 2);
    }
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const string query_test = "cat city -in"s;
        tuple<vector<string>, DocumentStatus> expected = srv.MatchDocument(query_test, 12);

        const vector<string> a = get<0>(expected);
        ASSERT_EQUAL(a.size(), 0);
    }
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const string query_test = "cat cat"s;
        tuple<vector<string>, DocumentStatus> expected = srv.MatchDocument(query_test, 12);

        const vector<string> a = get<0>(expected);
        ASSERT_EQUAL(a[0], "cat"s);
        ASSERT_EQUAL(a.size(), 1);
    }
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
        const string query_test = "cat"s;
        tuple<vector<string>, DocumentStatus> expected = srv.MatchDocument(query_test, 12);

        const vector<string> a = get<0>(expected);
        const DocumentStatus b = get<1>(expected);
        ASSERT_EQUAL(a[0], "cat"s);
        ASSERT_EQUAL(b, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(a.size(), 1);
    }
}

void TestSortRelevance() {
    const int doc_id1 = 12;
    const string content1 = "cat in the city";
    const vector<int> ratings1 = { 1, 2, 3 };
    const int doc_id2 = 13;
    const string content2 = "dog out woods city";
    const vector<int> ratings2 = { 1, 3, 5 };

    {
        SearchServer srv;
        srv.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        srv.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);

        vector<Document> result = srv.FindTopDocuments("city cat"s);
        ASSERT_EQUAL(result[0].id, 12);
        ASSERT_EQUAL(result.size(), 2);
    }
    {
        SearchServer srv;
        srv.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        srv.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);

        vector<Document> result = srv.FindTopDocuments("city dog"s);
        ASSERT_EQUAL(result[0].id, 13);
        ASSERT_EQUAL(result.size(), 2);
    }
    {
        SearchServer srv;
        srv.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        srv.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);

        vector<Document> result = srv.FindTopDocuments("dog"s);
        ASSERT_EQUAL(result[0].id, 13);
        ASSERT_EQUAL(result.size(), 1);
    }
    {
        SearchServer srv;
        srv.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        srv.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);

        vector<Document> result = srv.FindTopDocuments("dog cat -woods"s);
        ASSERT_EQUAL(result[0].id, 12);
        ASSERT_EQUAL(result.size(), 1);
    }
    {
        SearchServer srv;
        srv.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        srv.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);

        vector<Document> result = srv.FindTopDocuments("dog cat -city"s);
        ASSERT_EQUAL(result.size(), 0);
    }
}

void TestAverageRating() {
    const int doc_id = 12;
    const string content = "cat in the city";
    
    {
        SearchServer srv;
        const vector<int> ratings = { 1, 2, 3 };
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = srv.FindTopDocuments("cat");

        ASSERT_EQUAL(res.size(), 1);
        ASSERT_EQUAL(res[0].rating, 2);
    }    
    {
        SearchServer srv;
        const vector<int> ratings = { 1, 2, 4 };
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = srv.FindTopDocuments("cat");

        ASSERT_EQUAL(res.size(), 1);
        ASSERT_EQUAL(res[0].rating, 2);
    }    
    {
        SearchServer srv;
        const vector<int> ratings = { 1, 2, 5 };
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = srv.FindTopDocuments("cat");

        ASSERT_EQUAL(res.size(), 1);
        ASSERT_EQUAL(res[0].rating, 2);
    }    
    {
        SearchServer srv;
        const vector<int> ratings = { 1, 2, 6 };
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = srv.FindTopDocuments("cat");

        ASSERT_EQUAL(res.size(), 1);
        ASSERT_EQUAL(res[0].rating, 3);
    }    

    const int doc_id1 = 13;
    const string content1 = "dog out woods city";

    {
        SearchServer srv;
        const vector<int> ratings = { 0, 1, 2 };
        const vector<int> ratings1 = { 3, 4, 5 };
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        srv.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        vector<Document> res = srv.FindTopDocuments("city cat");

        ASSERT_EQUAL(res.size(), 2);
        ASSERT_EQUAL(res[0].rating, 1);
        ASSERT_EQUAL(res[1].rating, 4);
    }

}

void TestPredicat() {
    const int doc_id = 12, doc_id1 = 13;
    const string content = "cat in the city", content1 = "dog out woods city";
    vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        srv.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = srv.FindTopDocuments("cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });

        ASSERT_EQUAL(res.size(), 1);
        ASSERT_EQUAL(res[0].id, 12);
    }   
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        srv.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
        vector<Document> res = srv.FindTopDocuments("city"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });

        ASSERT_EQUAL(res.size(), 2);
        ASSERT_EQUAL(res[0].id, 12);
        ASSERT_EQUAL(res[1].id, 13);
    }
}
void TestStatus() {
    const int doc_id = 12, doc_id1 = 13;
    const string content = "cat in the city", content1 = "dog out woods city";
    vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        srv.AddDocument(doc_id1, content1, DocumentStatus::IRRELEVANT, ratings);

        vector<Document> res = srv.FindTopDocuments("city"s, DocumentStatus::ACTUAL);

        ASSERT_EQUAL(res.size(), 1);
        ASSERT_EQUAL(res[0].id, 12);
    }
}

void TestRelevance() {
    const int doc_id = 12, doc_id1 = 13;
    const string content = "cat in the city", content1 = "dog out woods city";
    vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer srv;
        srv.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        srv.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);

        vector<Document> res = srv.FindTopDocuments("cat city"s);

        ASSERT_EQUAL(res.size(), 2);
        double DELTA = (res[0].relevance - 0.173287);
        ASSERT_HINT(DELTA < 1e-6 && DELTA > -1e-6, "Relevance error"s);
        ASSERT_EQUAL(res[1].relevance, 0);
    }
}

/*
Разместите код остальных тестов здесь
*/

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatching);
    RUN_TEST(TestSortRelevance);
    RUN_TEST(TestAverageRating);
    RUN_TEST(TestPredicat);
    RUN_TEST(TestStatus);
    RUN_TEST(TestRelevance);
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
