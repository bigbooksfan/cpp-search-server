#if 1

//#define TEST_MODE

#include <execution>
#include <iostream>
#include <string>
#include <vector>
#include <random>

#include "log_duration.h"
#include "search_server.h"
#include "test_example_functions.h"

#ifdef TEST_MODE

using namespace std;
string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution(static_cast<int>('a'), static_cast<int>('z'))(generator));
    }
    return word;
}
vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}
string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, static_cast<int>(dictionary.size()) - 1)(generator)];
    }
    return query;
}
vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}
template <typename ExecutionPolicy>
void Test(string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    cout << total_relevance << endl;
}

#define TEST1(policy) Test(#policy, search_server, queries, execution::policy)
int main() {

    mt19937 generator;
    const auto dictionary = GenerateDictionary(generator, 100, 10);
    const auto documents = GenerateQueries(generator, dictionary, 10'00, 70);
    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(static_cast<int>(i), documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }
    const auto queries = GenerateQueries(generator, dictionary, 100, 70);
    TEST1(seq);
    TEST1(par);
}

#endif

#ifndef TEST_MODE
using namespace std;

//void PrintDocument(const Document& document) {
//    cout << "{ "s
//        << "document_id = "s << document.id << ", "s
//        << "relevance = "s << document.relevance << ", "s
//        << "rating = "s << document.rating << " }"s << endl;
//}

int main() {
    MyUnitTests::TestSearchServer();

    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const string& text : {
            "white cat and yellow hat"s,
            "curly cat curly tail"s,
            "nasty dog with big eyes"s,
            "nasty pigeon john"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }
    cout << "ACTUAL by default:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    // параллельная версия
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}
#endif

#endif

#if 0
#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <execution>

#include "log_duration.h"
#include "search_server.h"
#include "process_queries.h"
#include "test_example_functions.h"

int main() {
    MyUnitTests::TestSearchServer();
    return 0;
}

#endif

#if 0

#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "search_server.h"
#include "log_duration.h"
#include "process_queries.h"

using namespace std;

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution(static_cast<int>('a'), static_cast<int>('z'))(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename ExecutionPolicy>
void Test(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    const int document_count = search_server.GetDocumentCount();
    int word_count = 0;
    for (int id = 0; id < document_count; ++id) {
        const auto [words, status] = search_server.MatchDocument(policy, query, id);
        word_count += words.size();
    }
    cout << word_count << endl;
}

#define TEST(policy) Test(#policy, search_server, query, execution::policy)

int main() {
    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, 1000, 10);
    const auto documents = GenerateQueries(generator, dictionary, 1000, 70);

    const string query = GenerateQuery(generator, dictionary, 500, 0.1);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }

    TEST(seq);
    TEST(par);
}
#endif

#if 0

#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <execution>

#include "log_duration.h"
#include "search_server.h"
#include "process_queries.h"
#include "test_example_functions.h"

using namespace std;

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        //int in = uniform_int_distribution(static_cast<int>('a'), static_cast<int>('z'))(generator);
        //char c = static_cast<char>(in);
        word.push_back(uniform_int_distribution(static_cast<int>('a'), static_cast<int>('z'))(generator));
    }
    return word;
}

//int CountWords(string_view str) {
//    // подсчитайте количество слов,
//    // игнорируя начальные, конечные
//    // и подряд идущие пробелы
//
//    int i = transform_reduce(execution::par, str.begin() + 1, str.end(), str.begin(), 0, plus<>{}, [](char c1, char c2) { return c1 != ' ' && c2 == ' '; });
//
//    return str[0] == ' ' ? i : i + 1;
//    // некорректное решение, с которым сравнивается производительность
//    // return count(str.begin(), str.end(), ' ') + 1;
//}

vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int max_word_count) {
    const int word_count = uniform_int_distribution(1, max_word_count)(generator);
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        query += dictionary[uniform_int_distribution<int>(0, static_cast<int>(dictionary.size()) - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename ExecutionPolicy>
void Test1(string_view mark, SearchServer search_server, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    const int document_count = search_server.GetDocumentCount();
    for (int id = 0; id < document_count; ++id) {
        search_server.RemoveDocument(policy, id);
    }
    cout << search_server.GetDocumentCount() << endl;
}

#define TEST1(mode) Test1(#mode, search_server, execution::mode)

string GenerateQuery2(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, static_cast<int>(dictionary.size()) - 1)(generator)];
    }
    return query;
}

template <typename ExecutionPolicy>
void Test2(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    const int document_count = search_server.GetDocumentCount();
    int word_count = 0;
    for (int id = 0; id < document_count; ++id) {
        const auto [words, status] = search_server.MatchDocument(policy, query, id);
        word_count += static_cast<int>(words.size());
    }
    cout << word_count << endl;
}

#define TEST2(policy) Test2(#policy, search_server, query, execution::policy)

int main() {
    //MyUnitTests::TestSearchServer();

#if 0
    {
        SearchServer search_server("and with"sv);
        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
            ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
        }
        const vector<string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
        };
        id = 0;
        for (
            const auto& documents : ProcessQueries(search_server, queries)
            ) {
            cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << endl;
        }
    }
#endif

#if 0
    /////////////////////////
    /
        mt19937 generator;
        const auto dictionary = GenerateDictionary(generator, 2'000, 25);
        const auto documents = GenerateQueries(generator, dictionary, 20'000, 10);
        SearchServer search_server(dictionary[0]);
        for (int i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
        }
        const auto queries = GenerateQueries(generator, dictionary, 2'000, 7);
        MyUnitTests::TEST(ProcessQueries);
    }
#endif

#if 0
    {
        SearchServer search_server("and with"sv);
        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
            ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
        }
        const vector<string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
        };
        for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
            cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
        }
    }
#endif

#if 0
    {
        SearchServer search_server("and with"sv);

        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
            ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
        }

        const string query = "curly and funny"s;

        auto report = [&search_server, &query] {
            cout << search_server.GetDocumentCount() << " documents total, "s
                << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << endl;
        };

        report();
        // однопоточная версия
        search_server.RemoveDocument(5);
        report();
        // однопоточная версия
        search_server.RemoveDocument(execution::seq, 1);
        report();
        // многопоточная версия
        search_server.RemoveDocument(execution::par, 2);
        report();
    }
#endif

#if 0
    {
        mt19937 generator;

        const auto dictionary = GenerateDictionary(generator, 10/*'000*/, 25);
        const auto documents = GenerateQueries(generator, dictionary, 10/*'000*/, 100);

        //{
        //    SearchServer search_server(dictionary[0]);
        //    for (int i = 0; i < static_cast<int>(documents.size()); ++i) {
        //        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
        //    }

        //    TEST1(seq);
        //}
        {
            SearchServer search_server(dictionary[0]);
            for (int i = 0; i < static_cast<int>(documents.size()); ++i) {
                search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
            }

            TEST1(par);
        }
    }
#endif

#if 1
    /*
    {
        SearchServer search_server("and with"sv);

        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
            ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
        }

        const string query = "curly and funny -not"s;

        {
            const auto [words, status] = search_server.MatchDocument(query, 1);
            cout << words.size() << " words for document 1"s << endl;
            // 1 words for document 1
        }

        {
            const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
            cout << words.size() << " words for document 2"s << endl;
            // 2 words for document 2
        }

        {
            const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
            cout << words.size() << " words for document 3"s << endl;
            // 0 words for document 3
        }
    }
    */
    {
        mt19937 generator;

        const auto dictionary = GenerateDictionary(generator, 1000, 10);
        const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

        const string query = GenerateQuery2(generator, dictionary, 500, 0.1);

        SearchServer search_server(dictionary[0]);
        for (int i = 0; i < static_cast<int>(documents.size()); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
        }

        TEST2(seq);
        TEST2(par);
    }
#endif

    return 0;
}
#endif