//
// Created by Roman Titkov on 23.03.2022.
//

#ifndef LABS_WORDCONTEXT_H
#define LABS_WORDCONTEXT_H

#include <utility>
#include <vector>
#include "dictionary.h"
class WordContext {
public:
    wstring normalizedForm;
    double stability = 0;
    unordered_map <string, vector<int>> textEntries;

    WordContext(const wstring& normalizedForm) {
        this->normalizedForm = normalizedForm;
    }
};
#endif //LABS_WORDCONTEXT_H
