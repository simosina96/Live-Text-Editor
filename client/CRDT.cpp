//
// Created by simone on 05/08/19.
//

#include <ctime>
#include <cmath>
#include "CRDT.h"
#include <QDebug>


CRDT::CRDT() {
    this->structure = {};
}

void CRDT::setStructure(const std::vector<std::vector<Character>> &initialStructure) {
    if(initialStructure.empty()) {
        CRDT::structure = {};
    } else {
        CRDT::structure = initialStructure;
    }
}

const QString &CRDT::getSiteId() const {
    return siteId;
}

void CRDT::setSiteId(const QString &siteId) {
    CRDT::siteId = siteId;
}

Pos CRDT::insert(Character character) {

    Pos pos = this->findInsertPosition(character);
    this->insertChar(character, pos);

    return pos;
}

Pos CRDT::findInsertPosition(Character character) {
    // check if struct is empty or char is less than first char
    if (this->structure.empty() || character.compareTo(this->structure[0][0]) <= 0) {
        return Pos{ 0, 0 };
    }

    int minLine = 0;
    int totalLines = this->structure.size();
    int maxLine = totalLines - 1;
    std::vector<Character> lastLine = this->structure[maxLine];

    Character lastChar = lastLine[lastLine.size() - 1];

    // char is greater than all existing chars (insert at end)
    if (character.compareTo(lastChar) > 0) {
        return this->findEndPosition(lastChar, lastLine, totalLines);
    }

    // binary search
    while (minLine + 1 < maxLine) {
        int midLine = std::floor(minLine + (maxLine - minLine) / 2);
        std::vector<Character> currentLine = this->structure[midLine];
        lastChar = currentLine[currentLine.size() - 1];

        if (character.compareTo(lastChar) == 0) {
            return Pos{ midLine, (int) currentLine.size() - 1 };
        } else if (character.compareTo(lastChar) < 0) {
            maxLine = midLine;
        } else {
            minLine = midLine;
        }
    }

    // Check between min and max line.
    std::vector<Character> minCurrentLine = this->structure[minLine];
    Character minLastChar = minCurrentLine[minCurrentLine.size() - 1];
    std::vector<Character> maxCurrentLine = this->structure[maxLine];
    Character maxLastChar = maxCurrentLine[maxCurrentLine.size() - 1];

    if (character.compareTo(minLastChar) <= 0) {
        int charIdx = this->findInsertIndexInLine(character, minCurrentLine);
        return Pos { charIdx, minLine };
    } else {
        int charIdx = this->findInsertIndexInLine(character, maxCurrentLine);
        return Pos { charIdx, maxLine };
    }

}

Pos CRDT::findEndPosition(Character lastChar, std::vector<Character> lastLine, int totalLines) {
    if (lastChar.getValue() == '\n') {
        return Pos { 0, totalLines };
    } else {
        return Pos { (int) lastLine.size(), totalLines - 1 };
    }
}

int CRDT::findInsertIndexInLine(Character character, std::vector<Character> line) {

    int left = 0;
    int right = line.size() - 1;

    if (line.size() == 0 || character.compareTo(line[left]) < 0) {
        return left;
    } else if (character.compareTo(line[right]) > 0) {
        return this->structure.size();
    }

    while (left + 1 < right) {
        int mid = std::floor(left + (right - left) / 2);
        int compareNum = character.compareTo(line[mid]);
        if (compareNum == 0) {
            return mid;
        } else if (compareNum > 0) {
            left = mid;
        } else {
            right = mid;
        }
    }

    if (character.compareTo(line[left]) == 0) {
        return left;
    } else {
        return right;
    }

}

void CRDT::insertChar(Character character, Pos pos) {
    //qDebug() << "Char: " << character.getValue() << "inserted in pos " << pos.getLine() << pos.getCh();

    if (pos.getLine() == structure.size()) {
        structure.push_back(std::vector<Character> {}); // pushing a new line.
    }

    // if inserting a newline, split line into two lines.
    if (character.getValue() == '\n') {
        //qDebug() << "Splitting line into two lines";
        std::vector<Character> lineAfter(structure[pos.getLine()].begin() + pos.getCh(), structure[pos.getLine()].end()); // get line after.
        if (lineAfter.size() != 0) {
            //qDebug().noquote() << "There is something after the newLine inserted";
            structure[pos.getLine()].erase(structure[pos.getLine()].begin() + pos.getCh(), structure[pos.getLine()].end()); // delete line after.
            /*
            if(structure.size() <= pos.getLine()+1) {
                // the line + 1 does not exist.
                qDebug() << "line + 1 does not exist. Pushing back.";
                structure.push_back(lineAfter); // pushing back the line after.
            } else {
                // the line + 1 exists.
                qDebug() << "line + 1 exists, Inserting.";
                structure.insert(structure.begin() + pos.getLine() + 1, lineAfter.begin(), lineAfter.end());
            }
             */

            structure.insert(structure.begin() + pos.getLine() + 1, lineAfter);
        } else {
            //qDebug().noquote() << "There is nothing after the char \n inserted";
        }
    }

    structure[pos.getLine()].insert(structure[pos.getLine()].begin() + pos.getCh(), character); // insert the character in the pos.


    // print the structure for debugging
    qDebug() << "client/CRDT.cpp - insertChar()     ---------- STRUCTURE ----------";
    for (int i = 0; i < structure.size(); i++) {
        for (int j = 0; j < structure[i].size(); j++) {
            QDebug qD(QtDebugMsg);
            char val = structure[i][j].getValue();
            int counter = structure[i][j].getCounter();
            QString siteId = structure[i][j].getSiteId();
            QString value = ""; if(val == '\n') value += "\n"; else value += val;
            if(i == pos.getLine() && j == pos.getCh()) {
                qD << "                              ---> val:" << value << "  siteId: " << siteId << "  counter:" << counter << "  position:";
            } else {
                qD << "                                   val:" << value << "  siteId: " << siteId << "  counter:" << counter << "  position:";
            }
            std::vector<Identifier> identifier = structure[i][j].getPosition();
            for (Identifier id : identifier) {
                qD << id.getDigit();
            }
        }
    }
    qDebug() << ""; // newLine
}




std::vector<Character> CRDT::handleDelete(Pos startPos, Pos endPos) {
    bool newlineRemoved = false;
    std::vector<Character> removedChars;

    // for multi-line deletes
    if (startPos.getLine() != endPos.getLine()) {

        // delete chars on first line from startPos.ch to end of line
        newlineRemoved = true;
        removedChars = this->deleteMultipleLines(startPos, endPos);

        // single-line deletes
    } else {
        removedChars = this->deleteSingleLine(startPos, endPos);

        for(Character c : removedChars) {
            if(c.getValue() == '\n') newlineRemoved = true;
        }
    }

    this->removeEmptyLines();

    if (newlineRemoved && startPos.getCh() > 0) {
        //qDebug() << "need to merge line" << startPos.getLine();
        this->mergeLines(startPos.getLine());
    }

    // print the structure for debugging
    qDebug() << "client/CRDT.cpp - handleDelete()     ---------- STRUCTURE ----------";
    for (int i = 0; i < structure.size(); i++) {
        for (int j = 0; j < structure[i].size(); j++) {
            QDebug qD(QtDebugMsg);
            char val = structure[i][j].getValue();
            int counter = structure[i][j].getCounter();
            QString siteId = structure[i][j].getSiteId();
            QString value = ""; if(val == '\n') value += "\n"; else value += val;
            qD << "                                           val:" << value <<  "  siteId: " << siteId << "  counter:" << counter << "  position:";
            std::vector<Identifier> identifier = structure[i][j].getPosition();
            for (Identifier id : identifier) {
                qD << id.getDigit();
            }
        }
    }
    qDebug() << ""; // newLine

    return removedChars;
}

std::vector<Character> CRDT::deleteMultipleLines(Pos startPos, Pos endPos) {
    std::vector<Character> chars = {};

    if (this->structure[endPos.getLine()].size() > 0) {
        chars.insert(chars.end(), structure[endPos.getLine()].begin(), structure[endPos.getLine()].begin() + endPos.getCh());
        structure[endPos.getLine()].erase(structure[endPos.getLine()].begin(), structure[endPos.getLine()].begin() + endPos.getCh());
    }

    for (int line = endPos.getLine() - 1; line > startPos.getLine(); line--) {
        chars.insert(chars.end(), structure[line].begin(), structure[line].end());
        structure.erase(structure.begin() + line );
    }

    chars.insert(chars.end(), structure[startPos.getLine()].begin() + startPos.getCh(), structure[startPos.getLine()].end());
    structure[startPos.getLine()].erase(structure[startPos.getLine()].begin() + startPos.getCh(), structure[startPos.getLine()].end());

    return chars;
}

std::vector<Character> CRDT::deleteSingleLine(Pos startPos, Pos endPos) {
    // TODO check if correct
    int charNum = endPos.getCh() - startPos.getCh();
    //qDebug() << "client/CRDT.cpp - deleteSingleLine()     charNum: " << charNum;
    //qDebug() << "client/CRDT.cpp - deleteSingleLine()     startPos.getCh(): " << startPos.getCh();
    //qDebug() << "client/CRDT.cpp - deleteSingleLine()     startPos.getCh() + charNum: " << startPos.getCh() + charNum;
    //qDebug() << "client/CRDT.cpp - deleteSingleLine()     structure[startPos.getLine()].size(): " << structure[startPos.getLine()].size();
    if(structure[startPos.getLine()].size() < startPos.getCh() + charNum) {
        // TODO lanciare un'eccezione per evitare crash?
        qDebug() << "client/CRDT.cpp - deleteSingleLine()     ATTENZIONE: impossibile cancellare. Char/s non presente/i";
        qDebug() << ""; // newLine
    }
    std::vector<Character> chars {structure[startPos.getLine()].begin() + startPos.getCh(), structure[startPos.getLine()].begin() + startPos.getCh() + charNum};
    this->structure[startPos.getLine()].erase(structure[startPos.getLine()].begin() + startPos.getCh(), structure[startPos.getLine()].begin() + startPos.getCh() + charNum);

    return chars;
}




Pos CRDT::handleRemoteDelete(const Character &character) {
    Pos pos = this->findPosition(character);

    if (!pos) return pos;

    this->structure[pos.getLine()].erase(this->structure[pos.getLine()].begin() + pos.getCh());

    if (character.getValue() == '\n') {
        this->mergeLines(pos.getLine());
    }

    this->removeEmptyLines();

    // print the structure for debugging
    qDebug() << "client/CRDT.cpp - handleRemoteDelete()     ---------- STRUCTURE ----------";
    for (int i = 0; i < structure.size(); i++) {
        for (int j = 0; j < structure[i].size(); j++) {
            QDebug qD(QtDebugMsg);
            char val = structure[i][j].getValue();
            int counter = structure[i][j].getCounter();
            QString siteId = structure[i][j].getSiteId();
            QString value = ""; if(val == '\n') value += "\n"; else value += val;
            qD << "                                                 val:" << value << "  siteId: " << siteId << "  counter:" << counter << "  position:";
            std::vector<Identifier> identifier = structure[i][j].getPosition();
            for (Identifier id : identifier) {
                qD << id.getDigit();
            }
        }
    }
    qDebug() << ""; // newLine

    return pos;
}

Pos CRDT::findPosition(Character character) {
    // check if struct is empty or char is less than first char
    if (this->structure.empty() || character.compareTo(this->structure[0][0]) < 0) {
        return Pos {-1, -1}; // false obj
    }

    int minLine = 0;
    int totalLines = this->structure.size();
    int maxLine = totalLines - 1;
    std::vector<Character> lastLine = this->structure[maxLine];

    Character lastChar = lastLine[lastLine.size() - 1];

    // char is greater than all existing chars (insert at end)
    if (character.compareTo(lastChar) > 0) {
        return Pos {-1, -1}; // false obj
    }

    // binary search
    while (minLine + 1 < maxLine) {
        int midLine = std::floor(minLine + (maxLine - minLine) / 2);
        std::vector<Character> currentLine = this->structure[midLine];
        lastChar = currentLine[currentLine.size() - 1];

        if (character.compareTo(lastChar) == 0) {
            return Pos { (int) currentLine.size() - 1, midLine };
        } else if (character.compareTo(lastChar) < 0) {
            maxLine = midLine;
        } else {
            minLine = midLine;
        }
    }

    // Check between min and max line.
    std::vector<Character> minCurrentLine = this->structure[minLine];
    Character minLastChar = minCurrentLine[minCurrentLine.size() - 1];
    std::vector<Character> maxCurrentLine = this->structure[maxLine];
    Character maxLastChar = maxCurrentLine[maxCurrentLine.size() - 1];


    if (character.compareTo(minLastChar) <= 0) {
        int charIdx = this->findIndexInLine(character, minCurrentLine);
        return Pos { charIdx, minLine };
    } else {
        int charIdx = this->findIndexInLine(character, maxCurrentLine);
        return Pos { charIdx, maxLine };
    }
}

int CRDT::findIndexInLine(Character character, std::vector<Character> line) {

    int left = 0;
    int right = line.size() - 1;

    if (line.size() == 0 || character.compareTo(line[left]) < 0) {
        return left;
    } else if (character.compareTo(line[right]) > 0) {
        return this->structure.size();
    }

    while (left + 1 < right) {
        int mid = std::floor(left + (right - left) / 2);
        int compareNum = character.compareTo(line[mid]);

        if (compareNum == 0) {
            return mid;
        } else if (compareNum > 0) {
            left = mid;
        } else {
            right = mid;
        }
    }

    if (character.compareTo(line[left]) == 0) {
        return left;
    } else {
        return right;
    }

}

void CRDT::removeEmptyLines() {
    for (int line = 0; line < this->structure.size(); line++) {
        if (this->structure[line].size() == 0) {
            this->structure.erase(this->structure.begin() + line);
            line--;
        }
    }
}

void CRDT::mergeLines(int line) {
    if(structure.size() > line + 1 && structure[line + 1].size() > 0) {
        structure[line].insert(structure[line].end(), structure[line + 1].begin(), structure[line + 1].end());
        //qDebug() << "EREASING line" << line + 1 << " line size:" << structure[line+1].size();
        structure.erase(structure.begin() + line + 1);
    }
}

const QString CRDT::toText() {
    QString text = "";
    for(int i=0; i<structure.size(); i++) {
        for(int j=0; j<structure[i].size(); j++) {
            text += structure[i][j].getValue();
        }
    }
    return text;
}