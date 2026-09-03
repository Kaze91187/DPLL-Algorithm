#include "Global.h"

enum BasicClauseState {
    BASIC_UNRESOLVED = -1,
    BASIC_CONFLICT = 0,
    BASIC_SATISFIED = 1,
    BASIC_UNIT = 2
};

static BasicClauseState EvaluateBasicClause(
    const Headnode* clause,
    const int* assignment,
    int& unitLiteral
) {
    int unassigned = 0;
    unitLiteral = 0;

    for (const Datanode* node = clause->first;
         node != nullptr; node = node->next) {
        const int var = std::abs(node->data);
        const int value = assignment[var];

        if (value == noanswer) {
            ++unassigned;
            unitLiteral = node->data;
        } else if ((node->data > 0 && value == TRUE) ||
                   (node->data < 0 && value == FALSE)) {
            return BASIC_SATISFIED;
        }
    }

    if (unassigned == 0)
        return BASIC_CONFLICT;
    if (unassigned == 1)
        return BASIC_UNIT;
    return BASIC_UNRESOLVED;
}

static bool SolveBasic(
    const Headnode* formula,
    int varcount,
    int* assignment
) {
    while (true) {
        bool changed = false;

        for (const Headnode* clause = formula;
             clause != nullptr; clause = clause->next) {
            int unitLiteral = 0;
            const BasicClauseState state =
                EvaluateBasicClause(clause, assignment, unitLiteral);

            if (state == BASIC_CONFLICT)
                return false;

            if (state == BASIC_UNIT) {
                const int var = std::abs(unitLiteral);
                assignment[var] = unitLiteral > 0 ? TRUE : FALSE;
                changed = true;
            }
        }

        if (!changed)
            break;
    }

    int* frequency = new int[varcount + 1]();
    int branchvar = 0;

    for (const Headnode* clause = formula;
         clause != nullptr; clause = clause->next) {
        int unitLiteral = 0;
        if (EvaluateBasicClause(clause, assignment, unitLiteral)
            == BASIC_SATISFIED) {
            continue;
        }

        for (const Datanode* node = clause->first;
             node != nullptr; node = node->next) {
            const int var = std::abs(node->data);
            if (assignment[var] == noanswer) {
                ++frequency[var];
                if (frequency[var] > frequency[branchvar])
                    branchvar = var;
            }
        }
    }

    delete[] frequency;
    if (branchvar == 0)
        return true;

    int* saved = new int[varcount + 1];
    for (int i = 0; i <= varcount; ++i)
        saved[i] = assignment[i];

    assignment[branchvar] = TRUE;
    if (SolveBasic(formula, varcount, assignment)) {
        delete[] saved;
        return true;
    }

    for (int i = 0; i <= varcount; ++i)
        assignment[i] = saved[i];

    assignment[branchvar] = FALSE;
    if (SolveBasic(formula, varcount, assignment)) {
        delete[] saved;
        return true;
    }

    for (int i = 0; i <= varcount; ++i)
        assignment[i] = saved[i];
    delete[] saved;
    return false;
}

Status DPLLBasic(Headnode* formula, int* result, int varnum) {
    int* assignment = new int[varnum + 1];
    for (int i = 0; i <= varnum; ++i)
        assignment[i] = noanswer;

    const bool satisfiable = SolveBasic(formula, varnum, assignment);
    if (satisfiable) {
        for (int var = 1; var <= varnum; ++var) {
            result[var - 1] = assignment[var] == noanswer
                                  ? TRUE
                                  : assignment[var];
        }
    }

    delete[] assignment;
    return satisfiable ? TRUE : FALSE;
}
