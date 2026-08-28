//
// Created by PC on 2019/2/17.
//
#include "Global.h"

HeadNode* CreateClause(int &VARNUM,string &filename) {
    ifstream fis(filename);
    if(!fis){
        throw runtime_error("Cannot open CNF file: " + filename);
    }

    string line, format;
    int VarNum = 0;
    int ClauseNum = 0;
    while (getline(fis, line)) {
        if (line.empty() || line[0] == 'c')
            continue;
        istringstream header(line);
        header >> format;
        if (format == "p") {
            header >> format >> VarNum >> ClauseNum;
            if (format != "cnf")
                throw runtime_error("Only DIMACS CNF format is supported: " + filename);
            break;
        }
    }

    if (VarNum <= 0 || ClauseNum < 0)
        throw runtime_error("Invalid DIMACS header in CNF file: " + filename);

    HeadNode* HEAD = nullptr;
    HeadNode* headRear = nullptr;
    HeadNode* clause = new HeadNode;
    DataNode* dataRear = nullptr;
    int parsedClauses = 0;
    try {
        while (getline(fis, line)) {
            size_t first = line.find_first_not_of(" \t\r");
            if (first == string::npos || line[first] == 'c')
                continue;
            istringstream values(line.substr(first));
            int literal;
            while (values >> literal) {
                if (literal == 0) {
                    if (HEAD == nullptr) HEAD = clause;
                    else headRear->down = clause;
                    headRear = clause;
                    clause = new HeadNode;
                    dataRear = nullptr;
                    ++parsedClauses;
                } else {
                    if (abs(literal) > VarNum)
                        throw runtime_error("Literal outside declared variable range: " + to_string(literal));
                    DataNode* data = new DataNode;
                    data->data = literal;
                    if (dataRear == nullptr) clause->right = data;
                    else dataRear->next = data;
                    dataRear = data;
                    ++clause->Num;
                }
            }
            if (!values.eof())
                throw runtime_error("Invalid token in CNF file: " + filename);
        }
        if (clause->Num != 0)
            throw runtime_error("Last CNF clause is missing its terminating 0");
        delete clause;
        if (parsedClauses != ClauseNum)
            throw runtime_error("DIMACS clause count mismatch: declared " + to_string(ClauseNum) +
                                ", parsed " + to_string(parsedClauses));
    } catch (...) {
        DataNode* literal = clause->right;
        while (literal != nullptr) {
            DataNode* next = literal->next;
            delete literal;
            literal = next;
        }
        delete clause;
        DestroyFormula(HEAD);
        throw;
    }

    cout << "Parsed " << VarNum << " variables and " << ClauseNum
         << " clauses.\n";

    VARNUM = VarNum;
    return HEAD;
}
