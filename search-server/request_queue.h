#pragma once

#include <string>
#include <deque>
#include <algorithm>

//#include "document.h"
//#include "search_server.h"

//class RequestQueue {
//public:
//    explicit RequestQueue(const SearchServer& search_server) : search_server_for_requests(search_server) { }
//
//    template <typename DocumentPredicate>
//    void AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
//        if (!requests_.empty() && requests_.size() >= min_in_day_) {
//            requests_.pop_front();
//        }
//        AddLine(search_server_for_requests.FindTopDocuments(raw_query, document_predicate).empty());
//    }
//
//    void AddFindRequest(const std::string& raw_query, DocumentStatus status);
//    void AddFindRequest(const std::string& raw_query);
//
//    int GetNoResultRequests() const;
//
//private:
//    struct QueryResult {
//        int number;
//        bool empty;
//    };
//    std::deque<QueryResult> requests_;
//    const static int min_in_day_ = 1440;
//    const SearchServer& search_server_for_requests;
//
//    void AddLine(bool is_empty);
//};