#include <iostream>
#include <string>
#include <vector>
#include "filesystem"
#include <codecvt>
#include <fstream>
#include <sys/mman.h>
#include "dictionary.h"
#include "filemap.h"
#include "WordContext.h"
#include "Entry.h"
#include "json.hpp"
#include <chrono>

using namespace std;
using recursive_directory_iterator = std::__fs::filesystem::recursive_directory_iterator;

double K1 = 2;
double B = 0.75;

vector<string> getFilesFromDir(const string& dirPath) {
    vector<string> files;
    for (const auto& dirEntry : recursive_directory_iterator(dirPath))
        if (!(dirEntry.path().filename() == ".DS_Store"))
            files.push_back(dirEntry.path());
    return files;
}

WordContext* processContext(const vector<Word*>& wordVector, int windowSize,
                            unordered_map <wstring, WordContext*> *NGrams) {
    wstring normalizedForm;
    normalizedForm.reserve(windowSize * 8);

    vector<Word*> contextVector;
    for (int i = 0; i < windowSize; i++) {
        Word* word = wordVector[i];
        contextVector.push_back(word);
        normalizedForm += word->word + L' ';
    }
    normalizedForm.pop_back();

    return new WordContext(normalizedForm);
}

void addNgramEntryInText(WordContext* context,
                         unordered_map <wstring, WordContext*>* NGramms,
                         const string& filePath, int positiion,
                         unordered_map<wstring, vector<pair<wstring, bool>>> &relations) {
    wstring nornalForm = context->normalizedForm;
    if (NGramms->find(nornalForm) == NGramms->end())
        NGramms->emplace(nornalForm, context);

    auto &ngram= NGramms->at(nornalForm);
    if (ngram->textEntries.find(filePath) == ngram->textEntries.end())
        ngram->textEntries.emplace(filePath, vector<int>{});

    ngram->textEntries.at(filePath).push_back(positiion);

    if (relations.find(nornalForm) != relations.end()) {
        for (auto &synonimPair : relations[nornalForm]) {
            wstring synonim = synonimPair.first;
            auto* synonimContext = new WordContext(synonim);
            if (NGramms->find(synonim) == NGramms->end())
                NGramms->emplace(synonim, synonimContext);

            auto &synonimNgram= NGramms->at(synonim);
            if (synonimNgram->textEntries.find(filePath) == synonimNgram->textEntries.end())
                synonimNgram->textEntries.emplace(filePath, vector<int>{});
        }
    }
}

void handleWord(Word* word,
                vector<Word*>* leftWordContext,
                unordered_map <wstring, WordContext*> *NGrams,
                int windowSize, const string& filePath,
                const int wordPosition,
                unordered_map<wstring, vector<pair<wstring, bool>>> &relations) {
    if (windowSize <= leftWordContext->size()) {
        WordContext* phraseContext = processContext(*leftWordContext, windowSize, NGrams);
        addNgramEntryInText(phraseContext, NGrams, filePath, wordPosition, relations);
    }

    if (leftWordContext->size() == (windowSize + 1)) {
        leftWordContext->erase(leftWordContext->begin());
    }

    leftWordContext->push_back(word);
}

void handleFile(const string& filepath, const vector<Word*>& fileContent,
                unordered_map <wstring, vector<Word*>> *dictionary,
                unordered_map <wstring, WordContext*> *NGrams,
                int windowSize, unordered_map<wstring, vector<pair<wstring, bool>>> &relations) {
    vector<Word*> leftWordContext;

    int wordPosition = 0;
    for (Word* word : fileContent) {
        handleWord(word,&leftWordContext,
                   NGrams, windowSize, filepath, wordPosition, relations);
        wordPosition++;
    }
}

unordered_map <string, vector<Word*>> readAllTexts(const vector<string>& files,
                                                   unordered_map <wstring, vector<Word*>> *dictionary,
                                                   double *averageLength) {
    auto &f = std::use_facet<std::ctype<wchar_t>>(std::locale());
    unordered_map <string, vector<Word*>> fileContents;
    vector<wstring> leftStringNGrams;

    *averageLength = 0;
    for (const auto& filepath : files) {
        fileContents.emplace(filepath, vector<Word *>{});
        vector<Word*> fileContent = fileContents.at(filepath);

        size_t length;
        auto filePtr = map_file(filepath.c_str(), length);
        auto firstChar = filePtr;
        auto lastChar = filePtr + length;

        while (filePtr && filePtr != lastChar) {
            auto stringBegin = filePtr;
            filePtr = static_cast<char *>(memchr(filePtr, '\n', lastChar - filePtr));

            wstring line = wstring_convert<codecvt_utf8<wchar_t>>().from_bytes(stringBegin, filePtr);
            wstring const delims{L" :;.,!?() \r\n"};

            size_t beg, pos = 0;
            while ((beg = line.find_first_not_of(delims, pos)) != string::npos) {
                pos = line.find_first_of(delims, beg + 1);
                wstring wordStr = line.substr(beg, pos - beg);

                f.toupper(&wordStr[0], &wordStr[0] + wordStr.size());

                if (dictionary->find(wordStr) == dictionary->end()) {
                    Word *newWord = new Word();
                    newWord->word = wordStr;
                    newWord->partOfSpeech = L"UNKW";
                    dictionary->emplace(newWord->word, vector<Word *>{newWord});
                }

                vector<Word *> &words = dictionary->at(wordStr);
                fileContent.push_back(words.at(0));

                if (line[pos] == L'.' || line[pos] == L',' ||
                    line[pos] == L'!' || line[pos] == L'?') {
                    wstring punct;
                    punct.push_back(line[pos]);
                    if (dictionary->find(punct) == dictionary->end()) {
                        Word *newWord = new Word();
                        newWord->word = punct;
                        newWord->partOfSpeech = L"UNKW";
                        dictionary->emplace(newWord->word, vector<Word *>{newWord});
                    }

                    vector<Word *> &wordsPunct = dictionary->at(punct);
                    fileContent.push_back(wordsPunct.at(0));
                }
            }
            if (filePtr)
                filePtr++;
        }
        munmap(firstChar, length);
        fileContents.at(filepath) = fileContent;
        *averageLength += fileContent.size();
    }
    *averageLength /= (double) fileContents.size();
    return fileContents;
}

void handleRequest(unordered_map<wstring, vector<Entry*>> &phraseDescriptions,
                   unordered_map <wstring, vector<Word*>> *dictionary,
                   const wstring& request, const unordered_map <string, vector<Word*>>& filesContent,
                   double averageLength, unordered_map<wstring, vector<pair<wstring, bool>>> &relations) {
    auto &f = std::use_facet<std::ctype<wchar_t>>(std::locale());

    vector<wstring> requestWords;
    size_t beg, pos = 0;
    while ((beg = request.find_first_not_of(' ', pos)) != string::npos) {
        pos = request.find_first_of(' ', beg + 1);
        wstring wordStr = request.substr(beg, pos - beg);
        f.toupper(&wordStr[0], &wordStr[0] + wordStr.size());

        if (dictionary->find(wordStr) == dictionary->end()) {
            Word *newWord = new Word();
            newWord->word = wordStr;
            newWord->partOfSpeech = L"UNKW";
            dictionary->emplace(newWord->word, vector<Word*> {newWord});
        }

        vector<Word *> &words = dictionary->at(wordStr);
        wstring normalForm = words.at(0)->word;

        requestWords.push_back(normalForm);
    }

    vector<pair<string, double>> filenameToScore;
    for (const auto& filepair : filesContent) {
        string filename = filepair.first;
        auto fileContent = filepair.second;
        int fileSize = fileContent.size();
        double score = 0;
        for (const auto& requestWord : requestWords) {
            if (phraseDescriptions.find(requestWord) != phraseDescriptions.end()) {
                auto description  = phraseDescriptions.find(requestWord);
                for (auto &word: description->second) {
                    if (word->docName == filename) {
                        score += word->idf * (word->tf * (K1 + 1)) / (word->tf + K1 * (1 - B + B * fileSize / averageLength));
                    }
                }
                if (relations.find(requestWord) != relations.end()) {
                    auto wordsToHandle = relations[requestWord];
                    for (const auto& requestWordSynonim : wordsToHandle) {
                        if (phraseDescriptions.find(requestWordSynonim.first) != phraseDescriptions.end()) {
                            auto descriptionSynonim  = phraseDescriptions.find(requestWordSynonim.first);
                            for (auto &word: descriptionSynonim->second) {
                                if (word->docName == filename) {
                                    if (requestWordSynonim.second)
                                        score += 0.3 * word->idf * (word->tf * (K1 + 1)) / (word->tf + K1 * (1 - B + B * fileSize / averageLength));
                                    else
                                        score += 0.9 * word->idf * (word->tf * (K1 + 1)) / (word->tf + K1 * (1 - B + B * fileSize / averageLength));
                                }
                            }
                        }
                    }
                }
            }
        }
        filenameToScore.emplace_back(filename, score);
    }

    struct {
        bool operator()(const pair<string, double>& a, const pair<string, double>& b) const { return a.second > b.second; }
    } compDescription;
    sort(filenameToScore.begin(), filenameToScore.end(), compDescription);

    int OUTPUT_COUNT = 10;
    int count = 0;
    for (const auto& scorePair : filenameToScore) {
        if (count == OUTPUT_COUNT)
            break;
        count++;

        if (scorePair.second > 0) {
            string filename = scorePair.first;
            string filenameShort = scorePair.first;
            cout << "Result " << count << " | " << filenameShort.replace(0, 57, "") << " | Score: " << scorePair.second << " | File size: " << filesContent.at(filename).size() << endl;
            for (const auto& requestWord : requestWords) {
                if (phraseDescriptions.find(requestWord) != phraseDescriptions.end()) {
                    auto description = phraseDescriptions.find(requestWord);
                    for (auto &word: description->second) {
                        if (word->docName == filename) {
                            wcout << " - " << requestWord << " - Count: " << word->positions.size() << ", TF: " << word->tf << ", IDF: " << word->idf << endl;
                        }
                    }
                }
                if (relations.find(requestWord) != relations.end()) {
                    auto wordsToHandle = relations[requestWord];
                    for (const auto& requestWordSynonim : wordsToHandle) {
                        if (phraseDescriptions.find(requestWordSynonim.first) != phraseDescriptions.end()) {
                            auto descriptionSynonim  = phraseDescriptions.find(requestWordSynonim.first);
                            for (auto &word: descriptionSynonim->second) {
                                if (word->docName == filename && !word->positions.empty()) {
                                    wcout << "    - " << requestWordSynonim.first << " - Count: " << word->positions.size() << ", TF: " << word->tf << ", IDF: " << word->idf << endl;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void findTexts(const string& dictPath, const string& corpusPath,
               unordered_map<wstring, vector<pair<wstring, bool>>> &relations, vector<wstring> &requests) {
    auto dictionary = initDictionary(dictPath);
    vector<string> files = getFilesFromDir(corpusPath);

    unordered_map<wstring, WordContext *> phraseContexts;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    double averageLength;
    unordered_map <string, vector<Word*>> filesContent = readAllTexts(files, &dictionary, &averageLength);

    int windowSize = 1;
    while (true) {
        for (const string &filepath: files) {
            handleFile(filepath, filesContent.at(filepath), &dictionary,
                       &phraseContexts, windowSize, relations);
        }

        windowSize++;
        if (phraseContexts.empty() || windowSize == 4)
            break;
    }

    unordered_map<wstring, vector<Entry*>> phraseDescriptions;

    for (auto& pairContext : phraseContexts) {
        wstring normalForm = pairContext.first;
        for (auto &entry : pairContext.second->textEntries) {
            string filename = entry.first;

            if (phraseDescriptions.find(normalForm) == phraseDescriptions.end())
                phraseDescriptions.emplace(normalForm, vector<Entry*>{});

            double tf = (double) entry.second.size() / filesContent.at(filename).size();
            double idf = log10((double) files.size() / (double) pairContext.second->textEntries.size());

            auto phraseEntry = new Entry(filename, tf, idf, entry.second);
            phraseDescriptions.at(normalForm).push_back(phraseEntry);
        }
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "\nIndexation time = " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[s]" << std::endl;

    int requestNumber = 1;
    for (const wstring& request : requests) {
        wcout << endl << "Request " << requestNumber++ << ": " << request << endl;
        handleRequest(phraseDescriptions, &dictionary, request, filesContent, averageLength, relations);
    }
}

void loadTesaurus(const string& tesaurusPath, unordered_map<wstring, vector<pair<wstring, bool>>>  &relations) {
    nlohmann::json tesaurusJson;
    ifstream modelStream(tesaurusPath);
    modelStream >> tesaurusJson;

    for (const auto& synonimArray : tesaurusJson["synonims"]) {
        vector<wstring> synonims;
        for (const auto& synonim : synonimArray.items()) {
            string value = synonim.value().get<string>();
            wstring wideValue = wstring_convert<codecvt_utf8<wchar_t>>().from_bytes(value.data());
            synonims.push_back(wideValue);
        }

        for (const wstring& word : synonims) {
            vector<pair<wstring, bool>> synonimsForWord;
            for (const wstring& newSynonim : synonims) {
                if (newSynonim != word)
                    synonimsForWord.emplace_back(newSynonim, true);
            }
            relations.emplace(word, synonimsForWord);
        }
    }

    for (const auto& generalization : tesaurusJson["generalization"]) {
        for (const auto& synonim : generalization.items()) {
            string key = synonim.key();
            wstring wkey = wstring_convert<codecvt_utf8<wchar_t>>().from_bytes(key.data());
            for (const auto& word : synonim.value()) {
                string value = word.get<string>();
                wstring wideValue = wstring_convert<codecvt_utf8<wchar_t>>().from_bytes(value.data());

                if (relations.find(wkey) == relations.end())
                    relations.emplace(wkey, vector<pair<wstring, bool>> {});
                relations[wkey].emplace_back(wideValue, false);

                if (relations.find(wideValue) == relations.end())
                    relations.emplace(wideValue, vector<pair<wstring, bool>> {});
                relations[wideValue].emplace_back(wkey, false);
            }
        }
    }
}

void loadRequests(const string& requestsPath, vector<wstring> &requests) {
    nlohmann::json requestsJson;
    ifstream modelStream(requestsPath);
    modelStream >> requestsJson;

    for (const auto& request : requestsJson) {
        string value = request.get<string>();
        wstring wideValue = wstring_convert<codecvt_utf8<wchar_t>>().from_bytes(value.data());
        requests.push_back(wideValue);
    }
}

int main() {

    locale::global(locale("ru_RU.UTF-8"));
    wcout.imbue(locale("ru_RU.UTF-8"));

    string dictPath = "dict_opcorpora_clear.txt";
    string corpusPath = "/Users/titrom/Desktop/Computational Linguistics/Articles";
    string tesaurusPath = "/Users/titrom/Desktop/Computational Linguistics/Lab 5/tesaurus.json";
    string requestsPath = "/Users/titrom/Desktop/Computational Linguistics/Lab 5/requests.json";

    vector<wstring> requests;
    loadRequests(requestsPath, requests);

    unordered_map<wstring, vector<pair<wstring, bool>>> relations;
    loadTesaurus(tesaurusPath, relations);

    findTexts(dictPath, corpusPath, relations, requests);
    return 0;
}
