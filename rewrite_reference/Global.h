#ifndef REWRITE_REFERENCE_GLOBAL_H
#define REWRITE_REFERENCE_GLOBAL_H

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#define TRUE 1
#define FALSE 0
#define ROW 9
#define COL 9
#define NO_ANSWER -1

typedef int Status;

// 文字结点：data 保存正/负文字，next 指向同一子句的下一个文字。
typedef struct Datanode {
    int data = 0;
    struct Datanode* next = nullptr;
} Datanode;

// 子句结点：first 指向第一个文字，next 指向下一个子句。
typedef struct Headnode {
    int num = 0;
    Datanode* first = nullptr;
    struct Headnode* next = nullptr;
} Headnode;

Headnode* ParseCnf(const std::string& filename, int& variableCount);
void DestroyFormula(Headnode* formula);
Status DPLL(Headnode* formula, int* result, int variableCount);
int SolveStarSudoku(const std::string& filename);

#endif
