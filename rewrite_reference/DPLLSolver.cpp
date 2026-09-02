#include "Global.h"

enum ClauseState {
    CLAUSE_UNRESOLVED = -1,
    CLAUSE_CONFLICT = 0,
    CLAUSE_SATISFIED = 1,
    CLAUSE_UNIT = 2
};

void Destroy(Headnode* formula) {
    while (formula != nullptr) {
        Headnode* nextClause = formula->next;
        Datanode* literal = formula->first;

        while (literal != nullptr) {
            Datanode* nextLiteral = literal->next;
            delete literal;
            literal = nextLiteral;
        }

        delete formula;
        formula = nextClause;
    }
}

// 根据当前赋值判断一个子句的状态；单子句通过 unitLiteral 返回剩余文字。
static ClauseState EvaluateClause(
    const Headnode* clause,
    const int* assignment,
    int& unitLiteral
) {
    int unassignedCount = 0;
    unitLiteral = 0;

    for (const Datanode* node = clause->first; node != nullptr; node = node->next) {
        const int variable = std::abs(node->data);
        const int value = assignment[variable];

        if (value == noanswer) {
            ++unassignedCount;
            unitLiteral = node->data;
        } else if ((node->data > 0 && value == TRUE) ||
                   (node->data < 0 && value == FALSE)) {
            return CLAUSE_SATISFIED;
        }
    }

    if (unassignedCount == 0)
        return CLAUSE_CONFLICT;
    if (unassignedCount == 1)
        return CLAUSE_UNIT;
    return CLAUSE_UNRESOLVED;
}

static void Rollback(
    int* assignment,
    const int* trail,
    int& trailSize,
    int checkpoint
) {
    while (trailSize > checkpoint) {
        const int variable = trail[--trailSize];
        assignment[variable] = noanswer;
    }
}

static void Assign(
    int variable,
    int value,
    int* assignment,
    int* trail,
    int& trailSize
) {
    assignment[variable] = value;
    trail[trailSize++] = variable;
}

// 短子句中的文字权重更高；同时分别统计正、负文字来决定先试哪个方向。
static int SelectBranchVariable(
    const Headnode* formula,
    int variableCount,
    const int* assignment,
    int& preferredValue
) {
    unsigned long long* scores =
        new unsigned long long[(variableCount + 1) * 2]();
    unsigned long long* positiveScore = scores;
    unsigned long long* negativeScore = scores + variableCount + 1;

    for (const Headnode* clause = formula; clause != nullptr; clause = clause->next) {
        bool satisfied = false;
        int unassignedCount = 0;

        for (const Datanode* node = clause->first; node != nullptr; node = node->next) {
            const int variable = std::abs(node->data);
            const int value = assignment[variable];
            if ((node->data > 0 && value == TRUE) ||
                (node->data < 0 && value == FALSE)) {
                satisfied = true;
                break;
            }
            if (value == noanswer)
                ++unassignedCount;
        }

        if (satisfied || unassignedCount == 0)
            continue;

        // 近似 Jeroslow-Wang：长度越短权重越大，移位避免乘除法。
        const int shift = unassignedCount < 20 ? 20 - unassignedCount : 0;
        const unsigned long long weight = 1ULL << shift;

        for (const Datanode* node = clause->first; node != nullptr; node = node->next) {
            const int variable = std::abs(node->data);
            if (assignment[variable] != noanswer)
                continue;
            if (node->data > 0)
                positiveScore[variable] += weight;
            else
                negativeScore[variable] += weight;
        }
    }

    int branchVariable = 0;
    unsigned long long bestScore = 0;
    for (int variable = 1; variable <= variableCount; ++variable) {
        if (assignment[variable] != noanswer)
            continue;
        const unsigned long long total =
            positiveScore[variable] + negativeScore[variable];
        if (total > bestScore) {
            bestScore = total;
            branchVariable = variable;
        }
    }

    if (branchVariable != 0) {
        preferredValue = positiveScore[branchVariable] >= negativeScore[branchVariable]
                             ? TRUE
                             : FALSE;
    }

    delete[] scores;
    return branchVariable;
}

static bool SolveRecursive(
    const Headnode* formula,
    int variableCount,
    int* assignment,
    int* trail,
    int& trailSize
) {
    const int entryCheckpoint = trailSize;

    // 1. 反复执行单子句传播，直到没有产生新赋值。
    while (true) {
        bool changed = false;

        for (const Headnode* clause = formula; clause != nullptr; clause = clause->next) {
            int unitLiteral = 0;
            const ClauseState state = EvaluateClause(clause, assignment, unitLiteral);

            if (state == CLAUSE_CONFLICT) {
                Rollback(assignment, trail, trailSize, entryCheckpoint);
                return false;
            }

            if (state == CLAUSE_UNIT) {
                const int variable = std::abs(unitLiteral);
                const int requiredValue = unitLiteral > 0 ? TRUE : FALSE;
                Assign(variable, requiredValue, assignment, trail, trailSize);
                changed = true;
            }
        }

        if (!changed)
            break;
    }

    // 2. 选择高权重变量，并优先尝试得分更高的极性。
    int preferredValue = TRUE;
    const int branchVariable = SelectBranchVariable(
        formula, variableCount, assignment, preferredValue
    );

    if (branchVariable == 0)
        return true;

    // 3. 只记录赋值轨迹，通过检查点回溯，不再复制整个赋值数组。
    const int branchCheckpoint = trailSize;
    Assign(branchVariable, preferredValue, assignment, trail, trailSize);
    if (SolveRecursive(formula, variableCount, assignment, trail, trailSize))
        return true;
    Rollback(assignment, trail, trailSize, branchCheckpoint);

    Assign(branchVariable, preferredValue == TRUE ? FALSE : TRUE,
           assignment, trail, trailSize);
    if (SolveRecursive(formula, variableCount, assignment, trail, trailSize))
        return true;
    Rollback(assignment, trail, trailSize, branchCheckpoint);

    Rollback(assignment, trail, trailSize, entryCheckpoint);
    return false;
}

Status DPLL(Headnode* formula, int* result, int variableCount) {
    int* assignment = new int[variableCount + 1];
    int* trail = new int[variableCount + 1];
    int trailSize = 0;
    for (int i = 0; i <= variableCount; ++i)
        assignment[i] = noanswer;

    const bool satisfiable =
        SolveRecursive(formula, variableCount, assignment, trail, trailSize);

    if (satisfiable) {
        for (int variable = 1; variable <= variableCount; ++variable) {
            result[variable - 1] = assignment[variable] == noanswer
                                       ? TRUE
                                       : assignment[variable];
        }
    }

    delete[] trail;
    delete[] assignment;
    return satisfiable ? TRUE : FALSE;
}
