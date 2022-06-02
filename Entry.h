//
// Created by Roman Titkov on 05.05.2022.
//

#ifndef LAB_2_ENTRY_H
#define LAB_2_ENTRY_H

#include <utility>
#include <vector>
#include <string>

class Entry {
public:
    std::string docName;
    std::vector<int> positions;
    double tf;
    double idf;

    Entry(const std::string &docName, double tf, double idf, std::vector<int> positions) {
        this->docName = docName;
        this->tf = tf;
        this->idf = idf;
        this->positions = std::move(positions);
    }
};


#endif //LAB_2_ENTRY_H
