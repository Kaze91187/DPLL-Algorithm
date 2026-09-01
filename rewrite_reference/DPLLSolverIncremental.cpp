#include "Global.h"

typedef struct PackedFormula {
    int clauseCount = 0;
    int literalCount = 0;
    int* clauseOffsets = nullptr;
    int* literals = nullptr;
} PackedFormula;

typedef struct OccurrenceTable {
    int literalKindCount = 0;
    int* offsets = nullptr;
    int* clauseIndices = nullptr;
} OccurrenceTable;

typedef struct SolverState {
    int* assignment = nullptr;
    int* clauseUnassigned = nullptr;
    int* clauseSatisfied = nullptr;
    int* trail = nullptr;
    int trailSize = 0;
    int* queue = nullptr;
    int queueCapacity = 0;
    int* touchedClauses = nullptr;
    int* clauseStamp = nullptr;
    int currentStamp = 0;
    unsigned long long* scores = nullptr;
} SolverState;

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

static int LiteralIndex(int literal) {
    const int variable = std::abs(literal) - 1;
    return variable * 2 + (literal < 0 ? 1 : 0);
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
}

static OccurrenceTable BuildOccurrenceTable(
    const PackedFormula& formula,
    int variableCount
) {
    OccurrenceTable table;
    table.literalKindCount = variableCount * 2;
    table.offsets = new int[table.literalKindCount + 1]();

    for (int i = 0; i < formula.literalCount; ++i)
        ++table.offsets[LiteralIndex(formula.literals[i]) + 1];

    for (int i = 1; i <= table.literalKindCount; ++i)
        table.offsets[i] += table.offsets[i - 1];

    table.clauseIndices = formula.literalCount == 0
                              ? nullptr
                              : new int[formula.literalCount];
    int* cursor = new int[table.literalKindCount];
    for (int i = 0; i < table.literalKindCount; ++i)
        cursor[i] = table.offsets[i];

    for (int clause = 0; clause < formula.clauseCount; ++clause) {
        for (int i = formula.clauseOffsets[clause];
             i < formula.clauseOffsets[clause + 1]; ++i) {
            const int index = LiteralIndex(formula.literals[i]);
            table.clauseIndices[cursor[index]++] = clause;
        }
    }

    delete[] cursor;
    return table;
}

static void DestroyOccurrenceTable(OccurrenceTable& table) {
    delete[] table.offsets;
    delete[] table.clauseIndices;
    table.offsets = nullptr;
    table.clauseIndices = nullptr;
}

static int FindUnassignedLiteral(
    const PackedFormula& formula,
    int clause,
    const int* assignment
) {
    for (int i = formula.clauseOffsets[clause];
         i < formula.clauseOffsets[clause + 1]; ++i) {
        const int literal = formula.literals[i];
        if (assignment[std::abs(literal)] == NO_ANSWER)
            return literal;
    }
    return 0;
}

static void AddTouchedClause(
    int clause,
    SolverState& state,
    int& touchedCount
) {
    if (state.clauseStamp[clause] != state.currentStamp) {
        state.clauseStamp[clause] = state.currentStamp;
        state.touchedClauses[touchedCount++] = clause;
    }
}

static bool ApplyLiteral(
    int literal,
    const PackedFormula& formula,
    const OccurrenceTable& occurrences,
    SolverState& state,
    int& queueTail
) {
    const int variable = std::abs(literal);
    const int value = literal > 0 ? TRUE : FALSE;

    if (state.assignment[variable] != NO_ANSWER)
        return state.assignment[variable] == value;

    state.assignment[variable] = value;
    state.trail[state.trailSize++] = variable;

    ++state.currentStamp;
    if (state.currentStamp == 0) {
        for (int clause = 0; clause < formula.clauseCount; ++clause)
            state.clauseStamp[clause] = 0;
        state.currentStamp = 1;
    }
    int touchedCount = 0;

    const int positiveIndex = LiteralIndex(variable);
    for (int i = occurrences.offsets[positiveIndex];
         i < occurrences.offsets[positiveIndex + 1]; ++i) {
        const int clause = occurrences.clauseIndices[i];
        --state.clauseUnassigned[clause];
        if (value == TRUE)
            ++state.clauseSatisfied[clause];
        AddTouchedClause(clause, state, touchedCount);
    }

    const int negativeIndex = LiteralIndex(-variable);
    for (int i = occurrences.offsets[negativeIndex];
         i < occurrences.offsets[negativeIndex + 1]; ++i) {
        const int clause = occurrences.clauseIndices[i];
        --state.clauseUnassigned[clause];
        if (value == FALSE)
            ++state.clauseSatisfied[clause];
        AddTouchedClause(clause, state, touchedCount);
    }

    for (int i = 0; i < touchedCount; ++i) {
        const int clause = state.touchedClauses[i];
        if (state.clauseSatisfied[clause] > 0)
            continue;
        if (state.clauseUnassigned[clause] == 0)
            return false;
        if (state.clauseUnassigned[clause] == 1) {
            const int unitLiteral =
                FindUnassignedLiteral(formula, clause, state.assignment);
            if (unitLiteral == 0)
                return false;
            if (queueTail >= state.queueCapacity)
                throw std::runtime_error("Propagation queue overflow");
            state.queue[queueTail++] = unitLiteral;
        }
    }
    return true;
}

static bool Propagate(
    int firstLiteral,
    const PackedFormula& formula,
    const OccurrenceTable& occurrences,
    SolverState& state
) {
    int queueHead = 0;
    int queueTail = 0;
    if (firstLiteral != 0)
        state.queue[queueTail++] = firstLiteral;

    while (queueHead < queueTail) {
        if (!ApplyLiteral(state.queue[queueHead++], formula,
                          occurrences, state, queueTail))
            return false;
    }
    return true;
}

static void UndoTo(
    int checkpoint,
    const OccurrenceTable& occurrences,
    SolverState& state
) {
    while (state.trailSize > checkpoint) {
        const int variable = state.trail[--state.trailSize];
        const int value = state.assignment[variable];

        const int positiveIndex = LiteralIndex(variable);
        for (int i = occurrences.offsets[positiveIndex];
             i < occurrences.offsets[positiveIndex + 1]; ++i) {
            const int clause = occurrences.clauseIndices[i];
            ++state.clauseUnassigned[clause];
            if (value == TRUE)
                --state.clauseSatisfied[clause];
        }

        const int negativeIndex = LiteralIndex(-variable);
        for (int i = occurrences.offsets[negativeIndex];
             i < occurrences.offsets[negativeIndex + 1]; ++i) {
            const int clause = occurrences.clauseIndices[i];
            ++state.clauseUnassigned[clause];
            if (value == FALSE)
                --state.clauseSatisfied[clause];
        }
        state.assignment[variable] = NO_ANSWER;
    }
}

static int SelectBranchVariable(
    const PackedFormula& formula,
    int variableCount,
    SolverState& state,
    int& preferredValue
) {
    unsigned long long* positiveScore = state.scores;
    unsigned long long* negativeScore = state.scores + variableCount + 1;
    for (int variable = 0; variable <= variableCount; ++variable) {
        positiveScore[variable] = 0;
        negativeScore[variable] = 0;
    }

    for (int clause = 0; clause < formula.clauseCount; ++clause) {
        if (state.clauseSatisfied[clause] > 0)
            continue;

        const int count = state.clauseUnassigned[clause];
        const int shift = count < 20 ? 20 - count : 0;
        const unsigned long long weight = 1ULL << shift;

        for (int i = formula.clauseOffsets[clause];
             i < formula.clauseOffsets[clause + 1]; ++i) {
            const int literal = formula.literals[i];
            const int variable = std::abs(literal);
            if (state.assignment[variable] != NO_ANSWER)
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
        if (state.assignment[variable] != NO_ANSWER)
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
    const OccurrenceTable& occurrences,
    int variableCount,
    SolverState& state
) {
    int preferredValue = TRUE;
    const int branchVariable =
        SelectBranchVariable(formula, variableCount, state, preferredValue);
    if (branchVariable == 0)
        return true;

    const int checkpoint = state.trailSize;
    const int preferredLiteral = preferredValue == TRUE
                                     ? branchVariable
                                     : -branchVariable;
    if (Propagate(preferredLiteral, formula, occurrences, state) &&
        SolveRecursive(formula, occurrences, variableCount, state))
        return true;
    UndoTo(checkpoint, occurrences, state);

    if (Propagate(-preferredLiteral, formula, occurrences, state) &&
        SolveRecursive(formula, occurrences, variableCount, state))
        return true;
    UndoTo(checkpoint, occurrences, state);
    return false;
}

Status DPLL(Headnode* formula, int* result, int variableCount) {
    PackedFormula packed = PackFormula(formula);
    OccurrenceTable occurrences = BuildOccurrenceTable(packed, variableCount);
    SolverState state;

    state.assignment = new int[variableCount + 1];
    state.clauseUnassigned = new int[packed.clauseCount];
    state.clauseSatisfied = new int[packed.clauseCount]();
    state.trail = new int[variableCount + 1];
    state.queueCapacity = packed.clauseCount + variableCount + 1;
    state.queue = new int[state.queueCapacity];
    state.touchedClauses = new int[packed.clauseCount];
    state.clauseStamp = new int[packed.clauseCount]();
    state.scores = new unsigned long long[(variableCount + 1) * 2];

    for (int variable = 0; variable <= variableCount; ++variable)
        state.assignment[variable] = NO_ANSWER;

    bool valid = true;
    int initialQueueTail = 0;
    for (int clause = 0; clause < packed.clauseCount; ++clause) {
        const int length = packed.clauseOffsets[clause + 1]
                           - packed.clauseOffsets[clause];
        state.clauseUnassigned[clause] = length;
        if (length == 0) {
            valid = false;
        } else if (length == 1) {
            state.queue[initialQueueTail++] =
                packed.literals[packed.clauseOffsets[clause]];
        }
    }

    int queueHead = 0;
    while (valid && queueHead < initialQueueTail) {
        if (!ApplyLiteral(state.queue[queueHead++], packed, occurrences,
                          state, initialQueueTail))
            valid = false;
    }

    const bool satisfiable = valid &&
        SolveRecursive(packed, occurrences, variableCount, state);

    if (satisfiable) {
        for (int variable = 1; variable <= variableCount; ++variable) {
            result[variable - 1] = state.assignment[variable] == NO_ANSWER
                                       ? TRUE
                                       : state.assignment[variable];
        }
    }

    delete[] state.scores;
    delete[] state.clauseStamp;
    delete[] state.touchedClauses;
    delete[] state.queue;
    delete[] state.trail;
    delete[] state.clauseSatisfied;
    delete[] state.clauseUnassigned;
    delete[] state.assignment;
    DestroyOccurrenceTable(occurrences);
    DestroyPackedFormula(packed);
    return satisfiable ? TRUE : FALSE;
}
