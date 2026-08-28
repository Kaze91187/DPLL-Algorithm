//
// Created by PC on 2019/2/18.
//
#include "Global.h"

void DestroyFormula(HeadNode* LIST) {
    while (LIST != nullptr) {
        HeadNode* nextClause = LIST->down;
        DataNode* literal = LIST->right;
        while (literal != nullptr) {
            DataNode* nextLiteral = literal->next;
            delete literal;
            literal = nextLiteral;
        }
        delete LIST;
        LIST = nextClause;
    }
}

status IsEmptyClause(HeadNode* LIST) {
    HeadNode* PHead = LIST;
    while (PHead != nullptr) {
        if(PHead->Num == 0)
            return TRUE;
        PHead = PHead->down;
    }
    return FALSE;
}

HeadNode* IsSingleClause(HeadNode* Pfind) {
    while (Pfind != nullptr ) {
        if(Pfind->Num == 1)
            return Pfind;
        Pfind = Pfind->down;
    }
    return nullptr;
}

HeadNode* Duplication(HeadNode* LIST) { //此处检验传参正常，开始检查复制有无逻辑错误
    HeadNode* SrcHead = LIST;
    HeadNode* ReHead = new HeadNode;//新链表的头节点
    ReHead->Num = SrcHead->Num;//复制第一个头节点
    HeadNode* Phead = ReHead;//Phead创建头节点
    DataNode *ReData = new DataNode;//新链表的数据节点
    DataNode *FirstSrcData = SrcHead->right;//用于创建第一行的第一个数据节点
    ReData->data = FirstSrcData->data;//新链表的第一个数据节点的数值
    Phead->right = ReData;
    for (FirstSrcData = FirstSrcData->next;FirstSrcData != nullptr; FirstSrcData = FirstSrcData->next) {//第一行链表复制完成
        DataNode *NewDataNode = new DataNode;
        NewDataNode->data = FirstSrcData->data;
        ReData->next = NewDataNode;
        ReData = ReData->next;
    }
    //此下行数节点的复制 >=2th
    for(SrcHead = SrcHead->down; SrcHead != nullptr ; SrcHead = SrcHead->down) {
        HeadNode* NewHead = new HeadNode;
        DataNode* NewData = new DataNode;
        NewHead->Num = SrcHead->Num;
        Phead->down = NewHead;
        Phead = Phead->down;
        DataNode* SrcData = SrcHead->right;
        NewData->data = SrcData->data;
        Phead->right = NewData;//第一个数据节点
        for (SrcData = SrcData->next;SrcData != nullptr; SrcData = SrcData->next) {//此行剩下的数据节点
            DataNode* node = new DataNode;
            node->data = SrcData->data;
            NewData->next = node;
            NewData = NewData->next;
        }
        NewData->next = nullptr;
    }
    Phead->down = nullptr;

    return ReHead;
}

HeadNode* ADDSingleClause(HeadNode* LIST,int Var) { //所加的单子句位于链表的头
    HeadNode* AddHead = new HeadNode;
    DataNode* AddData = new DataNode;
    AddData->data = Var;
    AddData->next = nullptr;
    AddHead->right = AddData;
    AddHead->Num = 1;
    AddHead->down = LIST;
    LIST = AddHead;
    return LIST;
}

void DeleteDataNode(int temp,HeadNode *&LIST) {
    for (HeadNode* pHeadNode = LIST; pHeadNode != nullptr ; pHeadNode = pHeadNode->down)
        for (DataNode *rear = pHeadNode->right; rear != nullptr ; rear = rear->next) {
            if (rear->data == temp)  //相等删除整行
                DeleteHeadNode(pHeadNode,LIST);
            else if (abs(rear->data) == abs(temp)) { //仅仅是绝对值相等铲除该节点
                if(rear == pHeadNode->right) { //头节点删除
                    pHeadNode->right = rear->next;
                    pHeadNode->Num--;
                }
                else{ //删除普通节点
                    for (DataNode* front = pHeadNode->right; front != nullptr; front= front->next)
                        if(front->next == rear) {
                            front->next = rear->next;
                            pHeadNode->Num--;
                        }
                }
            }
        }
}

void DeleteHeadNode(HeadNode *Clause,HeadNode *&LIST) {
    if (!Clause) return;
    if(Clause == LIST)
        LIST = Clause->down;
    else {
        for (HeadNode *front = LIST; front != nullptr; front = front->down)
            if (front->down == Clause) {
                front->down = Clause->down;
            }
    }
}

void show(struct consequence *result,int VarNum) {
    cout<<"V ";
    for(int i = 0; i < VarNum; i++) {
        if (result[i].value == TRUE)
            cout<<i+1<<" ";
        else if(result[i].value == FALSE)
            cout<<-(i+1)<<" ";
        else
            cout<<(i+1)<<" ";//剩下一堆可true可false，就索性输出true
    }
    cout<<endl;
}

static int clauseState(HeadNode* clause, int* assignment, int& unitLiteral) {
    int unassignedCount = 0;
    unitLiteral = 0;
    for (DataNode* literal = clause->right; literal != nullptr; literal = literal->next) {
        int variable = abs(literal->data);
        int value = assignment[variable];
        if (value == -1) {
            unassignedCount++;
            unitLiteral = literal->data;
        } else if ((literal->data > 0 && value == TRUE) ||
                   (literal->data < 0 && value == FALSE)) {
            return TRUE;
        }
    }
    if (unassignedCount == 0)
        return FALSE;
    // -1 means unresolved; 0 is reserved for a conflicting (false) clause.
    return unassignedCount == 1 ? 2 : -1;
}

static bool solveDPLL(HeadNode* formula, int variableCount, int* assignment) {
    while (true) {
        bool changed = false;
        for (HeadNode* clause = formula; clause != nullptr; clause = clause->down) {
            int unitLiteral = 0;
            int state = clauseState(clause, assignment, unitLiteral);
            if (state == FALSE)
                return false;
            if (state == 2) {
                int variable = abs(unitLiteral);
                int requiredValue = unitLiteral > 0 ? TRUE : FALSE;
                if (assignment[variable] != -1 && assignment[variable] != requiredValue)
                    return false;
                if (assignment[variable] == -1) {
                    assignment[variable] = requiredValue;
                    changed = true;
                }
            }
        }
        if (!changed)
            break;
    }

    int* frequency = new int[variableCount + 1]();
    int branchVariable = 0;
    for (HeadNode* clause = formula; clause != nullptr; clause = clause->down) {
        int unitLiteral = 0;
        if (clauseState(clause, assignment, unitLiteral) == TRUE)
            continue;
        for (DataNode* literal = clause->right; literal != nullptr; literal = literal->next) {
            int variable = abs(literal->data);
            if (assignment[variable] == -1) {
                frequency[variable]++;
                if (frequency[variable] > frequency[branchVariable])
                    branchVariable = variable;
            }
        }
    }
    delete[] frequency;

    if (branchVariable == 0)
        return true;

    int* saved = new int[variableCount + 1];
    for (int i = 0; i <= variableCount; ++i)
        saved[i] = assignment[i];
    assignment[branchVariable] = TRUE;
    if (solveDPLL(formula, variableCount, assignment)) {
        delete[] saved;
        return true;
    }
    for (int i = 0; i <= variableCount; ++i)
        assignment[i] = saved[i];
    assignment[branchVariable] = FALSE;
    if (solveDPLL(formula, variableCount, assignment)) {
        delete[] saved;
        return true;
    }
    for (int i = 0; i <= variableCount; ++i)
        assignment[i] = saved[i];
    delete[] saved;
    return false;
}

status DPLL(HeadNode *LIST, consequence *result, int variableCount) {
    if (variableCount == 0)
        return LIST == nullptr || !IsEmptyClause(LIST);

    int* assignment = new int[variableCount + 1];
    for (int i = 0; i <= variableCount; ++i)
        assignment[i] = -1;
    bool satisfiable = solveDPLL(LIST, variableCount, assignment);
    if (satisfiable) {
        for (int i = 1; i <= variableCount; ++i)
            result[i - 1].value = assignment[i] == -1 ? TRUE : assignment[i];
    }
    delete[] assignment;
    return satisfiable ? TRUE : FALSE;
}
