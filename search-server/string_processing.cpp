#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
    std::vector<std::string_view> ret;
    
    size_t wordBegin = 0, symbolPos = 0, spacePos = 0;
    do {
        symbolPos   = text.find_first_not_of(' ', wordBegin);
        spacePos    = text.find_first_of    (' ', symbolPos);

        if (symbolPos == std::string_view::npos) {
            // no more symbols
            break;
        }

        // add word to vector
        std::string_view word = text.substr(symbolPos, spacePos - symbolPos);
        ret.push_back(word);

        if (spacePos == std::string_view::npos) {
            // no more spaces
            break;
        }
        // go dig more words
        wordBegin = spacePos;        
    } while (1);

    return ret;
}