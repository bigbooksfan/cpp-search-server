#pragma once

#include <iostream>

struct Document {
    int id = 0;
    double relevance = 0.0;
    int rating = 0;

    Document() : id(0), relevance(0.0), rating(0) { }

    Document(int id_in, double relevance_in, int rating_in) : id(id_in), relevance(relevance_in), rating(rating_in) { }

};

std::ostream& operator<< (std::ostream& os, const Document& doc);

enum class DocumentStatus {     // enum for statuses
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

void PrintDocument(const Document& document);

std::ostream& operator<<(std::ostream& os, DocumentStatus doc_status);