//
// Created by Roman Titkov on 22.03.2022.
//

#include "dictionary.h"
#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include "unordered_map"
#include "filesystem"
#include <codecvt>
#include <string>

using recursive_directory_iterator = std::__fs::filesystem::recursive_directory_iterator;

Word::Word() {}

Word::Word(const wstring& str) {
        int begin = 0;
        for (int pos = 0; pos < str.size(); pos++) {
            if (str[pos] == L' ') {
                if (begin == 0)
                    this->word = str.substr(begin, pos - begin);
                else
                    break;
//                    this->writeGrammeme(str.substr(begin, pos - begin));

                begin = pos + 1;
            }

        }
    }

void Word::writeGrammeme(const wstring& str) {
    if (str == L"NOUN" || str == L"ADJF" || str == L"ADJS" || str == L"COMP" ||
        str == L"VERB" || str == L"INFN" || str == L"PRTF" || str == L"PRTS" ||
        str == L"GRND" || str == L"NUMR" || str == L"ADVB" || str == L"NPRO" ||
        str == L"PRED" || str == L"PREP" || str == L"CONJ" || str == L"PRCL" ||
        str == L"INTJ") {
        this->partOfSpeech = str;
    } else
    if (str == L"anim" || str == L"inan") {
        this->anim = str;
    } else
    if (str == L"masc" || str == L"femn" || str == L"neut" || str == L"ms-f" ||
        str == L"GNdr") {
        this->gender = str;
    } else
    if (str == L"sing" || str == L"plur" || str == L"Sgtm" || str == L"Pltm" || str == L"Fixd") {
        this->number = str;
    } else
    if (str == L"nomn" || str == L"gent" || str == L"datv" || str == L"accs" ||
        str == L"ablt" || str == L"loct" || str == L"voct" || str == L"gen1" ||
        str == L"gen2" || str == L"acc2" || str == L"loc1" || str == L"loc2" ||
        str == L"Abbr" || str == L"Name" || str == L"Surn" || str == L"Patr" ||
        str == L"Geox" || str == L"Orgn" || str == L"Trad" || str == L"Subx" ||
        str == L"Supr" || str == L"Qual" || str == L"Apro" || str == L"Anum" ||
        str == L"Poss" || str == L"V-ey" || str == L"V-oy" || str == L"Cmp2" ||
        str == L"V-ej") {
        this->Case = str;
    } else
    if (str == L"perf" || str == L"impf") {
        this->aspc = str;
    } else
    if (str == L"tran" || str == L"intr" || str == L"Impe" || str == L"Impx" ||
        str == L"Mult" || str == L"Refl") {
        this->trns = str;
    } else
    if (str == L"1per" || str == L"2per" || str == L"3per") {
        this->pers = str;
    } else
    if (str == L"pres" || str == L"past" || str == L"futr") {
        this->tens = str;
    } else
    if (str == L"indc" || str == L"impr") {
        this->mood = str;
    } else
    if (str == L"incl" || str == L"excl") {
        this->invi = str;
    } else
    if (str == L"actv" || str == L"pssv" || str == L"Infr" || str == L"Slng" ||
        str == L"Arch" || str == L"Litr" || str == L"Erro" || str == L"Dist" ||
        str == L"Ques" || str == L"Dmns" || str == L"Prnt" || str == L"V-be" ||
        str == L"V-en" || str == L"V-ie" || str == L"V-bi" || str == L"Fimp" ||
        str == L"Prdx" || str == L"Coun" || str == L"Coll" || str == L"V-sh" ||
        str == L"Af-p" || str == L"Inmx" || str == L"Vpre" || str == L"Anph" ||
        str == L"Init" || str == L"Adjx" || str == L"Ms-f" || str == L"Hypo") {
        this->voic = str;
    }
}

wstring getWord(const wstring &line) {
    int endPosition = 0;
    while (line.at(endPosition) != L' ')
        endPosition++;

    return line.substr(0, endPosition);
}

unordered_map <wstring, vector<Word*>> initDictionary(const string& filename) {
    cout << "Initializing dictionary...\n";
    unordered_map <wstring, vector<Word*>> dictionary;
    wifstream infile(filename);
    int wordCount = 0;
    Word *initWord = nullptr;

    size_t length;
    auto filePtr = map_file(filename.c_str(), length);
    auto lastChar = filePtr + length;

    while (filePtr && filePtr != lastChar) {
        auto stringBegin = filePtr;
        filePtr = static_cast<char *>(memchr(filePtr, '\n', lastChar - filePtr));
        if (filePtr)
            filePtr++;
        else
            break;

        if (filePtr - stringBegin < 10) {
            initWord = nullptr;
            continue;
        }

        wstring line = wstring_convert<codecvt_utf8<wchar_t>>().from_bytes(stringBegin, filePtr - 1);

        if (initWord == nullptr) {
            initWord = new Word(line);
        }

        wstring word = getWord(line);

        if (dictionary.find(word) != dictionary.end()) {
            bool wordExist = false;
            for (Word *existWord : dictionary.at(word)) {
                if (existWord == initWord) {
                    wordExist = true;
                    break;
                }
            }
            if (!wordExist)
                dictionary[word].push_back(initWord);
        } else {
            dictionary.emplace(word, vector<Word*>{initWord});
        }
        if (wordCount % 500000 == 0) {
            cout << "Words inserted: " << wordCount << endl;
        }
        wordCount++;
    }

    cout << "Dictionary initialized.\n";
    return dictionary;
}