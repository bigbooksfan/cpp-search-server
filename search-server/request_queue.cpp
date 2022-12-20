#include "request_queue.h"

void RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus stat) {
    return RequestQueue::AddFindRequest(raw_query, [stat](int document_id, DocumentStatus status, int rating) { return status == stat; });
}

void RequestQueue::AddFindRequest(const std::string& raw_query) {
    return RequestQueue::AddFindRequest(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
}

int RequestQueue::GetNoResultRequests() const {
    return std::count_if(requests_.begin(), requests_.end(), 
        [](QueryResult a) { return a.empty_; });
}

void RequestQueue::AddLine(bool is_empty) {
    int i = requests_.empty() ? 0 : requests_.back().number_ + 1;
    QueryResult a;
    a.number_ = i;
    a.empty_ = is_empty;
    requests_.push_back(a);
}