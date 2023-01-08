
#include "request_queue.h"
#include "read_input_functions.h"
#include "paginator.h"
#include "remove_duplicates.h"
#include "tests.h"

int main() {

    MyUnitTests::TestSearchServer();
    std::cerr << std::string{ "Search server testing finished" } << std::endl;

    SearchServer search_server(std::string{ "and in at" });
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, std::string{ "curly cat curly tail" }, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, std::string{ "curly dog and fancy collar" }, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, std::string{ "big cat fancy collar " }, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, std::string{ "big dog sparrow Eugene" }, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, std::string{ "big dog sparrow Vasiliy" }, DocumentStatus::ACTUAL, { 1, 1, 1 });
    search_server.AddDocument(6, std::string{ "big dog sparrow Vasiliy" }, DocumentStatus::ACTUAL, { 1, 1, 1 });

    for (int i = 10; i < 100; ++i) {
        search_server.AddDocument(i, std::string{ "big dog sparrow Vasiliy" }, DocumentStatus::ACTUAL, { 1, 1, 1 });
    }

    {
        //LOG_DURATION_STREAM("Remove duplicates", std::cerr);
        RemoveDuplicates(search_server);
    }
    return 0;
}
