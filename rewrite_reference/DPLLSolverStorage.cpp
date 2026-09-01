#include "Global.h"

enum ClauseState {
    CLAUSE_UNRESOLVED = -1,
    CLAUSE_CONFLICT = 0,
    CLAUSE_SATISFIED = 1,
    CLAUSE_UNIT = 2
};

// 连续公式存储：第 i 个子句的文字位于
// literals[clauseOffsets[i] ... clauseOffsets[i + 1])。
typedef struct PackedFormula {
    int clauseCount = 0;
    int literalCount = 0;
    int* clauseOffsets = nullptr;
    int* literals = nullptr;
} PackedFormula;

void DestroyFormula(Headnode* formula) {
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

static PackedFormula PackFormula(const Headnode* formula) {
    PackedFormula packed;

    for (const Headnode* clause = formula; clause != nullptr; clause = clause->next) {
        ++packed.clauseCount;
        packed.literalCount += clause->num;
    }

    packed.clauseOffsets = new int[packed.clauseCount + 1];
    packed.literals = packed.literalCount == 0
                          ? nullptr
                          : new int[packed.literalCount];

    int clauseIndex = 0;
    int literalIndex = 0;
    for (const Headnode* clause = formula; clause != nullptr; clause = clause->next) {
        packed.clauseOffsets[clauseIndex++] = literalIndex;
        for (const Datanode* node = clause->first; node != nullptr; node = node->next)
            packed.literals[literalIndex++] = node->data;
    }
    packed.clauseOffsets[packed.clauseCount] = literalIndex;
    return packed;
}

static void DestroyPackedFormula(PackedFormula& formula) {
    delete[] formula.clauseOffsets;
    delete[] formula.literals;
    formula.clauseOffsets = nullptr;
    formula.literals = nullptr;
    formula.clauseCount = 0;
    formula.literalCount = 0;
}

static ClauseState EvaluateClause(
    const PackedFormula& formula,
    int clauseIndex,
    const int* assignment,
    int& unitLiteral
) {
    int unassignedCount = 0;
    unitLiteral = 0;

    const int begin = formula.clauseOffsets[clauseIndex];
    const int end = formula.clauseOffsets[clauseIndex + 1];
    for (int i = begin; i < end; ++i) {
        const int literal = formula.literals[i];
        const int value = assignment[std::abs(literal)];

        if (value == NO_ANSWER) {
            ++unassignedCount;
            unitLiteral = literal;
        } else if ((literal > 0 && value == TRUE) ||
                   (literal < 0 && value == FALSE)) {
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
        assignment[variable] = NO_ANSWER;
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

static int SelectBranchVariable(
    const PackedFormula& formula,
    int variableCount,
    const int* assignment,
    unsigned long long* scores,
    int& preferredValue
) {
    unsigned long long* positiveScore = scores;
    unsigned long long* negativeScore = scores + variableCount + 1;

    // 复用DPLL入口处一次性申请的评分数组，避免递归中反复new/delete。
    for (int variable = 0; variable <= variableCount; ++variable) {
        positiveScore[variable] = 0;
        negativeScore[variable] = 0;
    }

    for (int clause = 0; clause < formula.clauseCount; ++clause) {
        const int begin = formula.clauseOffsets[clause];
        const int end = formula.clauseOffsets[clause + 1];
        bool satisfied = false;
        int unassignedCount = 0;

        for (int i = begin; i < end; ++i) {
            const int literal = formula.literals[i];
            const int value = assignment[std::abs(literal)];
            if ((literal > 0 && value == TRUE) ||
                (literal < 0 && value == FALSE)) {
                satisfied = true;
                break;
            }
            if (value == NO_ANSWER)
                ++unassignedCount;
        }

        if (satisfied || unassignedCount == 0)
            continue;

        const int shift = unassignedCount < 20 ? 20 - unassignedCount : 0;
        const unsigned long long weight = 1ULL << shift;

        for (int i = begin; i < end; ++i) {
            const int literal = formula.literals[i];
            const int variable = std::abs(literal);
            if (assignment[variable] != NO_ANSWER)
                continue;
            if (literal > 0)
                positiveScore[variable] += weight;
            else
                negativeScore[variable] += weight;
        }
    }

    int branchVariable = 0;
    unsigned long long bestScore = 0;
    for (int variable = 1; variable <= variableCount; ++variable) {
        if (assignment[variable] != NO_ANSWER)
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
    return branchVariable;
}

static bool SolveRecursive(
    const PackedFormula& formula,
    int variableCount,
    int* assignment,
    int* trail,
    int& trailSize,
    unsigned long long* scores
) {
    const int entryCheckpoint = trailSize;

    while (true) {
        bool changed = false;
        for (int clause = 0; clause < formula.clauseCount; ++clause) {
            int unitLiteral = 0;
            const ClauseState state =
                EvaluateClause(formula, clause, assignment, unitLiteral);

            if (state == CLAUSE_CONFLICT) {
                Rollback(assignment, trail, trailSize, entryCheckpoint);
                return false;
            }

            if (state == CLAUSE_UNIT) {
                Assign(std::abs(unitLiteral), unitLiteral > 0 ? TRUE : FALSE,
                       assignment, trail, trailSize);
                changed = true;
            }
        }
        if (!changed)
            break;
    }

    int preferredValue = TRUE;
    const int branchVariable = SelectBranchVariable(
        formula, variableCount, assignment, scores, preferredValue
    );
    if (branchVariable == 0)
        return true;

    const int branchCheckpoint = trailSize;
    Assign(branchVariable, preferredValue, assignment, trail, trailSize);
    if (SolveRecursive(formula, variableCount, assignment, trail, trailSize, scores))
        return true;
    Rollback(assignment, trail, trailSize, branchCheckpoint);

    Assign(branchVariable, preferredValue == TRUE ? FALSE : TRUE,
           assignment, trail, trailSize);
    if (SolveRecursive(formula, variableCount, assignment, trail, trailSize, scores))
        return true;
    Rollback(assignment, trail, trailSize, branchCheckpoint);

    Rollback(assignment, trail, trailSize, entryCheckpoint);
    return false;
}

Status DPLL(Headnode* formula, int* result, int variableCount) {
    PackedFormula packed = PackFormula(formula);
    int* assignment = new int[variableCount + 1];
    int* trail = new int[variableCount + 1];
    unsigned long long* scores =
        new unsigned long long[(variableCount + 1) * 2];
    int trailSize = 0;

    for (int i = 0; i <= variableCount; ++i)
        assignment[i] = NO_ANSWER;

    const bool satisfiable = SolveRecursive(
        packed, variableCount, assignment, trail, trailSize, scores
    );

    if (satisfiable) {
        for (int variable = 1; variable <= variableCount; ++variable) {
            result[variable - 1] = assignment[variable] == NO_ANSWER
                                       ? TRUE
                                       : assignment[variable];
        }
    }

    delete[] scores;
    delete[] trail;
    delete[] assignment;
    DestroyPackedFormula(packed);
    return satisfiable ? TRUE : FALSE;
}
