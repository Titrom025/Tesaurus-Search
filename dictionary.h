//
// Created by Roman Titkov on 22.03.2022.
//

#ifndef LABS_DICTIONARY_H
#define LABS_DICTIONARY_H

#include <string>
#include "filemap.h"
#include "set"
#include "unordered_map"

using namespace std;

class Word {
public:
    int totalEntryCount = 0;
    set <string> textEntry;
    wstring word;
    wstring partOfSpeech;
    wstring anim;
    wstring gender;
    wstring number;
    wstring Case;
    wstring aspc;
    wstring trns;
    wstring pers;
    wstring tens;
    wstring mood;
    wstring invi;
    wstring voic;

    Word();
    Word(const std::__1::basic_string<wchar_t, std::__1::char_traits<wchar_t>, std::__1::allocator<wchar_t>> &str);

    void writeGrammeme(const std::wstring &str);
};

unordered_map <wstring, vector<Word*>> initDictionary(const string& filename);
#endif //LABS_DICTIONARY_H
