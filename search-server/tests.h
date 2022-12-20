#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <tuple>
#include <set>
#include <cassert>

#include "search_server.h"

namespace MyUnitTests {

    using std::string;
    using std::cerr;

    using namespace std;            // need it here to avoid massive amount of ""s errors

    template <typename T, typename U>
    void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file, const string& func, unsigned line, const string& hint) {
        if (t != u) {
            cerr << boolalpha;
            cerr << file << "(" << line << "): " << func << ": ";
            cerr << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
            cerr << t << " != " << u << ".";
            if (!hint.empty()) {
                cerr << " Hint: " << hint;
            }
            cerr << std::endl;
            abort();
        }
    }

    void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
        const string& hint) {
        if (!value) {
            cerr << file << "(" << line << "): " << func << ": ";
            cerr << "ASSERT(" << expr_str << ") failed.";
            if (!hint.empty()) {
                cerr << " Hint: " << hint;
            }
            cerr << std::endl;
            abort();
        }
    }

    template <typename T>
    void RunTestImpl(const T& func, const string& func_name) {
        func();
        cerr << func_name << " OK\n";
    }

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, "")

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, "")

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

#define RUN_TEST(func) RunTestImpl((func), #func)


    // -------- Начало модульных тестов поисковой системы ----------

    // Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов

#if 1
    void TestExcludeStopWordsFromAddedDocumentContent() {
        const int doc_id = 42;
        const string content = "cat in the city";
        const std::vector<int> ratings = { 1, 2, 3 };
        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            std::vector<Document> res = server.FindTopDocuments("cat");
            ASSERT_EQUAL(res.size(), 1u);
            const Document& doc0 = res[0];
            ASSERT_EQUAL(doc0.id, doc_id);
        }

        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            std::vector<Document> res = server.FindTopDocuments("in");
            ASSERT_HINT(res.empty(), "top words must be excluded from documents");
        }
    }

#endif
#if 1

    void TestExcludeDocumentsByMinusWords() {
        const int doc_id = 12;
        const string content = "cat in the city";
        const std::vector<int> ratings = { 1, 2, 3 };
        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            std::vector<Document> res = server.FindTopDocuments("cat -city");
            ASSERT_EQUAL(res.size(), 0u);
        }
        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            std::vector<Document> res = server.FindTopDocuments("-city");
            ASSERT_EQUAL(res.size(), 0u);
        }
        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            std::vector<Document> res = server.FindTopDocuments("-city -city");
            ASSERT_EQUAL(res.size(), 0u);
        }
        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            std::vector<Document> res = server.FindTopDocuments("city -city");
            ASSERT_EQUAL(res.size(), 0u);
        }
        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            std::vector<Document> res = server.FindTopDocuments("city -a");
            ASSERT_EQUAL(res.size(), 1u);
        }
    }

#endif
#if 1

    void TestMatchingDocumentsByQuerry() {
        const int doc_id = 12;
        const string content = "cat in the city";
        const std::vector<int> ratings = { 1, 2, 3 };
        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            const string query_test = "cat";
            std::tuple<std::vector<string>, DocumentStatus> expected = server.MatchDocument(query_test, 12);

            auto& [a, b] = expected;

            //std::vector<string> a;
            //DocumentStatus b;
            //tie(a, b) = expected;       // tuple decomposing

            ASSERT_EQUAL(a.size(), 1u);
            ASSERT_EQUAL(a[0], "cat");
            ASSERT_EQUAL(b, DocumentStatus::ACTUAL);
        }
        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            const string query_test = "cat city";
            std::tuple<std::vector<string>, DocumentStatus> expected = server.MatchDocument(query_test, 12);

            const std::vector<string> a = std::get<0>(expected);
            ASSERT_EQUAL(a.size(), 2u);
            ASSERT_EQUAL(a[0], "cat");
            ASSERT_EQUAL(a[1], "city");
        }
        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            const string query_test = "cat city -in";
            std::tuple<std::vector<string>, DocumentStatus> expected = server.MatchDocument(query_test, 12);

            const std::vector<string>& a = std::get<0>(expected);
            ASSERT_EQUAL(a.size(), 2u);
        }
        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            const string query_test = "cat cat";
            std::tuple<std::vector<string>, DocumentStatus> expected = server.MatchDocument(query_test, 12);

            const std::vector<string> a = std::get<0>(expected);
            ASSERT_EQUAL(a.size(), 1u);
            ASSERT_EQUAL(a[0], "cat");
        }

        try {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            std::tuple<std::vector<string>, DocumentStatus> expected = server.MatchDocument("cat --cat", 12);
            ASSERT_HINT(false, "Check MatchDocument() and CheckDoubleMinus()");
        }
        catch (const std::invalid_argument& e) {
            // Do nothing
        }
        catch (...) {
            ASSERT_HINT(false, "Check MatchDocument() and CheckDoubleMinus()! Wrong exception has been throwed");
        }

        try {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            std::tuple<std::vector<string>, DocumentStatus> expected = server.MatchDocument("cat -", 12);
            ASSERT_HINT(false, "Check MatchDocument() and CheckNoMinusWord()");
        }
        catch (const std::invalid_argument& e) {
            // Do nothing
        }
        catch (...) {
            ASSERT_HINT(false, "Check MatchDocument() and CheckNoMinusWord()! Wrong exception has been throwed");
        }

        try {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, "city-cat out in", DocumentStatus::ACTUAL, ratings);
            const string query_test = "city-cat";
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
        const std::vector<int> ratings1 = { 1, 2, 3 };
        const int doc_id2 = 13;
        const string content2 = "dog out woods city";
        const std::vector<int> ratings2 = { 1, 3, 5 };
        const int doc_id3 = 14;
        const string content3 = "cat the city";
        const std::vector<int> ratings3 = { 2, 3, 4 };
        const int doc_id4 = 15;
        const string content4 = "wolf sea boat";
        const std::vector<int> ratings4 = { 6, 7, 8 };

        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
            server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
            server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
            server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);

            std::vector<Document> result = server.FindTopDocuments("city cat");
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
            SearchServer server(std::string{ "in the" });
            const std::vector<int> ratings = { 1, 2, 3 };
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            std::vector<Document> res = server.FindTopDocuments("cat");
            ASSERT_EQUAL(res.size(), 1u);
            int test_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
            ASSERT_EQUAL(res[0].rating, test_rating);
        }
        {
            SearchServer server(std::string{ "in the" });            // negative ratings
            const std::vector<int> ratings = { -1, -2, -4 };
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            std::vector<Document> res = server.FindTopDocuments("cat");
            ASSERT_EQUAL(res.size(), 1u);
            int test_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
            ASSERT_EQUAL(res[0].rating, test_rating);
        }
        {
            SearchServer server(std::string{ "in the" });        // Empty ratings
            std::vector<int> ratings;
            ratings.clear();
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            std::vector<Document> res = server.FindTopDocuments("cat");
            ASSERT_EQUAL(res.size(), 1u);
            ASSERT_EQUAL(res[0].rating, 0);
        }

        const int doc_id1 = 13;
        const string content1 = "dog out woods city";

        {
            SearchServer server(std::string{ "in the" });
            const std::vector<int> ratings = { 0, 1, 2 };
            const std::vector<int> ratings1 = { 3, 4, 5 };
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
            std::vector<Document> res = server.FindTopDocuments("city cat");
            ASSERT_EQUAL(res.size(), 2u);
            ASSERT_EQUAL(res[0].rating, 1);
            ASSERT_EQUAL(res[1].rating, 4);
        }
    }

    void TestFindingDocumentByPredicate() {
        const int doc_id = 12, doc_id1 = 13;
        const string content = "cat in the city", content1 = "dog out woods city";
        std::vector<int> ratings = { 1, 2, 3 };
        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
            std::vector<Document> res = server.FindTopDocuments("cat", [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
            ASSERT_EQUAL(res.size(), 1u);
            ASSERT_EQUAL(res[0].id, 12);
        }
        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
            std::vector<Document> res = server.FindTopDocuments("city", [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
            ASSERT_EQUAL(res.size(), 2u);
            ASSERT_EQUAL(res[0].id, 12);
            ASSERT_EQUAL(res[1].id, 13);
        }
    }

    void TestFindingDocumentByStatus() {
        const int doc_id = 12, doc_id1 = 13;
        const string content = "cat in the city", content1 = "dog out woods city";
        std::vector<int> ratings = { 1, 2, 3 };
        {
            SearchServer server(std::string{ "in the" });
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            server.AddDocument(doc_id1, content1, DocumentStatus::IRRELEVANT, ratings);
            std::vector<Document> res = server.FindTopDocuments("city", DocumentStatus::ACTUAL);
            ASSERT_EQUAL(res.size(), 1u);
            ASSERT_EQUAL(res[0].id, 12);
        }
    }

    void TestRelevanceCalculation() {
        const int doc_id = 12, doc_id1 = 13;
        const string content = "cat in the city"s, content1 = "dog out woods city"s;
        std::vector<int> ratings = { 1, 2, 3 };
        {
            SearchServer server("AAA BBB   ccc  "s);
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
            std::vector<Document> res = server.FindTopDocuments("cat city"s);
            ASSERT_EQUAL(res.size(), 2u);

            double test_idf_cat = log(2.0 / 1.0); // ( doc count / docs with word "cat" )
            double test_idf_city = log(2.0 / 2.0); // ( doc count / docs with word "city" )      log(1) == 0
            double test_tf_cat_doc0 = 1.0 / 4.0;     // ( word "cat" count / number of words ) in doc 0
            double test_tf_cat_doc1 = 0.0 / 4.0;     // ( word "cat" count / number of words ) in doc 1
            double test_tf_city_doc0 = 1.0 / 4.0;     // ( word "city" count / number of words ) in doc 0
            double test_tf_city_doc1 = 1.0 / 4.0;     // ( word "city" count / number of words ) in doc 1

            double test_relevance_doc0 = test_idf_cat * test_tf_cat_doc0 + test_idf_city * test_tf_city_doc0;        // 0.17328679513998632
            double DELTA0 = std::abs((res[0].relevance - test_relevance_doc0));
            ASSERT_HINT(DELTA0 < 1e-6, "Relevance calculation error! Doc relevance (" + std::to_string(res[1].relevance) + ") != expected (" + std::to_string(test_relevance_doc0) + ")");

            double test_relevance_doc1 = test_idf_cat * test_tf_cat_doc1 + test_idf_city * test_tf_city_doc1;        // 0
            double DELTA1 = std::abs((res[1].relevance - test_relevance_doc1));
            ASSERT_HINT(DELTA1 < 1e-6, "Relevance calculation error! Doc relevance (" + std::to_string(res[1].relevance) + ") != expected (" + std::to_string(test_relevance_doc1) + ")");
        }
    }

    void TestServerConstructingByContqainers() {
        const string content = "cat in the city";
        const string content1 = "dog out woods city";

        {
            const std::vector<string> test_stop_words = { "in", "the" };
            SearchServer server(test_stop_words);
            server.AddDocument(12, content, DocumentStatus::ACTUAL, { 1, 2, 3 });
            server.AddDocument(13, content1, DocumentStatus::ACTUAL, { 2, 3, 4 });
            std::vector<Document> res = server.FindTopDocuments("cat");
            ASSERT_EQUAL_HINT(res.size(), 1u, "erver construction via std::vector<string> fails");
            std::vector<Document> res1 = server.FindTopDocuments("city");
            ASSERT_EQUAL_HINT(res1.size(), 2u, "erver construction via std::vector<string> fails");
        }
        {
            const std::set<string> test_stop_words = { "in", "the" };
            SearchServer server(test_stop_words);
            server.AddDocument(12, content, DocumentStatus::ACTUAL, { 1, 2, 3 });
            server.AddDocument(13, content1, DocumentStatus::ACTUAL, { 2, 3, 4 });
            std::vector<Document> res = server.FindTopDocuments("cat");
            ASSERT_EQUAL_HINT(res.size(), 1u, "erver construction via std::vector<string> fails");
            std::vector<Document> res1 = server.FindTopDocuments("city");
            ASSERT_EQUAL_HINT(res1.size(), 2u, "erver construction via std::vector<string> fails");
        }
        {
            const std::vector<string> test_stop_words = {};
            SearchServer server(test_stop_words);
            server.AddDocument(12, content, DocumentStatus::ACTUAL, { 1, 2, 3 });
            std::vector<Document> res = server.FindTopDocuments("in");
            ASSERT_EQUAL_HINT(res.size(), 1u, "Check server constructing with empty stop-words std::vector<string>");
        }

    }

#endif
#if 1

    void TestAddDocuments() {
        const string content = "cat in the city";
        const std::vector<string> test_stop_words = { "in", "the" };

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
            ASSERT_HINT(false, "Check bool AddDocument()! Negative ID don't causes throwing invalid_argument");
        }
        catch (const std::invalid_argument& e) {
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
            server.AddDocument(12, "dog out of woods", DocumentStatus::ACTUAL, { 1, 2, 3 });
            ASSERT_HINT(false, "Check bool AddDocument()! Double ID, exception hasn't been throwed");
        }
        catch (const std::invalid_argument& e) {
            // Do nothing
        }
        catch (...) {
            ASSERT_HINT(false, "Check bool AddDocument()! Double ID, wrong exception has been throwed");
        }

        try {
            SearchServer server(test_stop_words);
            server.AddDocument(12, "cat in the c\x12ity", DocumentStatus::ACTUAL, { 1, 2, 3 });
            ASSERT_HINT(false, "Check bool AddDocument()! Special symbols, exception hasn't been throwed");
        }
        catch (const std::invalid_argument& e) {
            // Everything is OK
        }
        catch (...) {
            ASSERT_HINT(false, " Check AddDocument()! Special symbols in querry causes wrong exception");
        }

        try {
            SearchServer server(test_stop_words);
            server.AddDocument(0, "белый кот и модный ошейник", DocumentStatus::ACTUAL, { 1, 2, 3 });
        }
        catch (...) {
            ASSERT_HINT(false, "Check bool AddDocument()! Russian letters has throwed exception");
        }

        try {
            SearchServer server(test_stop_words);
            server.AddDocument(12, "белый кот и мо\x12дный ошейник", DocumentStatus::ACTUAL, { 1, 2, 3 });
            ASSERT_HINT(false, "Check bool AddDocument()! Special symbols in russian querry, exception hasn't been throwed");
        }
        catch (const std::invalid_argument& e) {
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
        const std::vector<string> test_stop_words = { "in", "the" };

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
            ASSERT_HINT(false, "Check GetDocumentId()! Out of range check might be missed");
        }
        catch (const std::out_of_range& e) {
            // Do nothing
        }
        catch (...) {
            ASSERT_HINT(false, "Check GetDocumentId()! Out of range ID, wrong exception has been throwed");
        }

        try {
            server.GetDocumentId(-6);
            ASSERT_HINT(false, "Check GetDocumentId()! Negative doc number check might be missed");
        }
        catch (const std::out_of_range& e) {
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
}