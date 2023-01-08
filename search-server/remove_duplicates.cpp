#include "remove_duplicates.h"

#include <vector>
#include <iostream>

void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> word_sets;
    std::vector<int> ids_to_remove;

    std::set<std::string> doc_words;

    for (const int doc_id : search_server) {
        for (const std::pair<std::string, double>& i : search_server.GetWordFrequencies(doc_id)) {
            doc_words.insert(i.first);
        }
        if (word_sets.count(doc_words) == 0) {
            word_sets.insert(doc_words);
        }
        else {
            ids_to_remove.push_back(doc_id);
        }
        doc_words.clear();
    }

    for (int i : ids_to_remove) {
        search_server.RemoveDocument(i);
        std::cout << "Found duplicate document id " << i << std::endl;
    }
}
