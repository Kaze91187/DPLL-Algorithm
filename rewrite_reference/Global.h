#ifndef REWRITE_REFERENCE_GLOBAL_H
#define REWRITE_REFERENCE_GLOBAL_H

#include <iostream>
#include <fstream>
#include <ctime>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstdlib>
#include <string>

#define TRUE 1
#define FALSE 0
#define ROW 9     // 行
#define COL 9     // 列
#define noanswer -1

typedef int Status;

// 文字结点
typedef struct Datanode {
    int data = 0;
    struct Datanode* next = nullptr;
} Datanode;

// 子句结点
typedef struct Headnode {
    int num = 0;
    Datanode* first = nullptr;
    struct Headnode* next = nullptr;
} Headnode;

Headnode* Create(const std::string& filename, int& varnum);
void Destroy(Headnode* formula);
Status DPLL(Headnode* formula, int* result, int varnum);
Status DPLLBasic(Headnode* formula, int* result, int varnum);
Status SolveStarPuzzle(const std::string& puzzle, int answer[ROW][COL]);
int Solvestar(const std::string& filename);
int Playstar(const std::string& filename);
int Testmode(const std::string& programPath,
             const std::string* files,
             int filecount,
             unsigned int timeoutMs);

#endif
