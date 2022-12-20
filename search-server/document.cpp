#include "document.h"

std::ostream& operator<< (std::ostream& os, const Document& doc) {
    os << "{ "
        << "document_id = " << doc.id << ", "
        << "relevance = " << doc.relevance << ", "
        << "rating = " << doc.rating
        << " }";// << std::endl;
    return os;
}

void PrintDocument(const Document& document) {
    std::cout << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating
        << " }" << std::endl;
}

std::ostream& operator<<(std::ostream& os, DocumentStatus doc_status) {
    if (doc_status == DocumentStatus::ACTUAL) os << "actual";
    if (doc_status == DocumentStatus::BANNED) os << "banned";
    if (doc_status == DocumentStatus::IRRELEVANT) os << "irrelevant";
    if (doc_status == DocumentStatus::REMOVED) os << "removed";
    return os;
}