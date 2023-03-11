//#include "request_queue.h"

//void RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus stat) {
//    return RequestQueue::AddFindRequest(raw_query,
//        [stat](int document_id, DocumentStatus status, int rating) { return status == stat; });
//}
//
//void RequestQueue::AddFindRequest(const std::string& raw_query) {
//    return RequestQueue::AddFindRequest(raw_query, DocumentStatus::ACTUAL);
//}
//
//int RequestQueue::GetNoResultRequests() const {
//    return static_cast<int>(std::count_if(requests_.begin(), requests_.end(),
//        [](QueryResult a) { return a.empty; }));
//}
//
//void RequestQueue::AddLine(bool is_empty) {
//    int i = requests_.empty() ? 0 : requests_.back().number + 1;
//    QueryResult a;
//    a.number = i;
//    a.empty = is_empty;
//    requests_.push_back(a);
//}