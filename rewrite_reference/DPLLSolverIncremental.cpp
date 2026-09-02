#include "Global.h"

typedef struct PackedFormula {
    int clauseCount = 0;
    int literalCount = 0;
    int* clauseOffsets = nullptr;
    int* literals = nullptr;
} PackedFormula;

typedef struct OccurrenceTable {
    int kindcount = 0;
    int* offsets = nullptr;
    int* clauseidx = nullptr;
} OccurrenceTable;

typedef struct SolverState {
    int* assignment = nullptr;
    int* clauseUnassigned = nullptr;
    int* clauseSatisfied = nullptr;
    int* trail = nullptr;
    int trailSize = 0;
    int* queue = nullptr;
    int queuemax = 0;
    int* touchedClauses = nullptr;
    int* clauseStamp = nullptr;
    int currentStamp = 0;
    unsigned long long* scores = nullptr;
} SolverState;

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

static int Literalidx(int literal) {
    const int var = std::abs(literal) - 1;
    return var * 2 + (literal < 0 ? 1 : 0);
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

    int clauseidx = 0;
    int literalidx = 0;
    for (const Headnode* clause = formula; clause != nullptr; clause = clause->next) {
        packed.clauseOffsets[clauseidx++] = literalidx;
        for (const Datanode* node = clause->first; node != nullptr; node = node->next)
            packed.literals[literalidx++] = node->data;
    }
    packed.clauseOffsets[packed.clauseCount] = literalidx;
    return packed;
}

static void DestroyPF(PackedFormula& formula) {
    delete[] formula.clauseOffsets;
    delete[] formula.literals;
    formula.clauseOffsets = nullptr;
    formula.literals = nullptr;
}

static OccurrenceTable BuildOccurrenceTable(
    const PackedFormula& formula,
    int varcount
) {
    OccurrenceTable table;
    table.kindcount = varcount * 2;
    table.offsets = new int[table.kindcount + 1]();

    for (int i = 0; i < formula.literalCount; ++i)
        ++table.offsets[Literalidx(formula.literals[i]) + 1];

    for (int i = 1; i <= table.kindcount; ++i)
        table.offsets[i] += table.offsets[i - 1];

    table.clauseidx = formula.literalCount == 0
                          ? nullptr
                          : new int[formula.literalCount];
    int* cursor = new int[table.kindcount];
    for (int i = 0; i < table.kindcount; ++i)
        cursor[i] = table.offsets[i];

    for (int clause = 0; clause < formula.clauseCount; ++clause) {
        for (int i = formula.clauseOffsets[clause];
             i < formula.clauseOffsets[clause + 1]; ++i) {
            const int idx = Literalidx(formula.literals[i]);
            table.clauseidx[cursor[idx]++] = clause;
        }
    }

    delete[] cursor;
    return table;
}

static void DestroyOT(OccurrenceTable& table) {
    delete[] table.offsets;
    delete[] table.clauseidx;
    table.offsets = nullptr;
    table.clauseidx = nullptr;
}

static int FindUnassignedLiteral(
    const PackedFormula& formula,
    int clause,
    const int* assignment
) {
    for (int i = formula.clauseOffsets[clause];
         i < formula.clauseOffsets[clause + 1]; ++i) {
        const int literal = formula.literals[i];
        if (assignment[std::abs(literal)] == noanswer)
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
    const int var = std::abs(literal);
    const int value = literal > 0 ? TRUE : FALSE;

    if (state.assignment[var] != noanswer)
        return state.assignment[var] == value;

    state.assignment[var] = value;
    state.trail[state.trailSize++] = var;

    ++state.currentStamp;
    if (state.currentStamp == 0) {
        for (int clause = 0; clause < formula.clauseCount; ++clause)
            state.clauseStamp[clause] = 0;
        state.currentStamp = 1;
    }
    int touchedCount = 0;

    const int posidx = Literalidx(var);
    for (int i = occurrences.offsets[posidx];
         i < occurrences.offsets[posidx + 1]; ++i) {
        const int clause = occurrences.clauseidx[i];
        --state.clauseUnassigned[clause];
        if (value == TRUE)
            ++state.clauseSatisfied[clause];
        AddTouchedClause(clause, state, touchedCount);
    }

    const int negidx = Literalidx(-var);
    for (int i = occurrences.offsets[negidx];
         i < occurrences.offsets[negidx + 1]; ++i) {
        const int clause = occurrences.clauseidx[i];
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
            if (queueTail >= state.queuemax)
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
        const int var = state.trail[--state.trailSize];
        const int value = state.assignment[var];

        const int posidx = Literalidx(var);
        for (int i = occurrences.offsets[posidx];
             i < occurrences.offsets[posidx + 1]; ++i) {
            const int clause = occurrences.clauseidx[i];
            ++state.clauseUnassigned[clause];
            if (value == TRUE)
                --state.clauseSatisfied[clause];
        }

        const int negidx = Literalidx(-var);
        for (int i = occurrences.offsets[negidx];
             i < occurrences.offsets[negidx + 1]; ++i) {
            const int clause = occurrences.clauseidx[i];
            ++state.clauseUnassigned[clause];
            if (value == FALSE)
                --state.clauseSatisfied[clause];
        }
        state.assignment[var] = noanswer;
    }
}

static int SelectBranchvar(
    const PackedFormula& formula,
    int varcount,
    SolverState& state,
    int& preferredValue
) {
    unsigned long long* posScore = state.scores;
    unsigned long long* negScore = state.scores + varcount + 1;
    for (int var = 0; var <= varcount; ++var) {
        posScore[var] = 0;
        negScore[var] = 0;
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
            const int var = std::abs(literal);
            if (state.assignment[var] != noanswer)
                continue;
            if (literal > 0)
                posScore[var] += weight;
            else
                negScore[var] += weight;
        }
    }

    int branchvar = 0;
    unsigned long long bestScore = 0;
    for (int var = 1; var <= varcount; ++var) {
        if (state.assignment[var] != noanswer)
            continue;
        const unsigned long long total =
            posScore[var] + negScore[var];
        if (total > bestScore) {
            bestScore = total;
            branchvar = var;
        }
    }

    if (branchvar != 0) {
        preferredValue = posScore[branchvar] >= negScore[branchvar]
                             ? TRUE
                             : FALSE;
    }
    return branchvar;
}

static bool SolveRecursive(
    const PackedFormula& formula,
    const OccurrenceTable& occurrences,
    int varcount,
    SolverState& state
) {
    int preferredValue = TRUE;
    const int branchvar =
        SelectBranchvar(formula, varcount, state, preferredValue);
    if (branchvar == 0)
        return true;

    const int checkpoint = state.trailSize;
    const int preferredLiteral = preferredValue == TRUE
                                     ? branchvar
                                     : -branchvar;
    if (Propagate(preferredLiteral, formula, occurrences, state) &&
        SolveRecursive(formula, occurrences, varcount, state))
        return true;
    UndoTo(checkpoint, occurrences, state);

    if (Propagate(-preferredLiteral, formula, occurrences, state) &&
        SolveRecursive(formula, occurrences, varcount, state))
        return true;
    UndoTo(checkpoint, occurrences, state);
    return false;
}

Status DPLL(Headnode* formula, int* result, int varcount) {
    PackedFormula packed = PackFormula(formula);
    OccurrenceTable occurrences = BuildOccurrenceTable(packed, varcount);
    SolverState state;

    state.assignment = new int[varcount + 1];
    state.clauseUnassigned = new int[packed.clauseCount];
    state.clauseSatisfied = new int[packed.clauseCount]();
    state.trail = new int[varcount + 1];
    state.queuemax = packed.clauseCount + varcount + 1;
    state.queue = new int[state.queuemax];
    state.touchedClauses = new int[packed.clauseCount];
    state.clauseStamp = new int[packed.clauseCount]();
    state.scores = new unsigned long long[(varcount + 1) * 2];

    for (int var = 0; var <= varcount; ++var)
        state.assignment[var] = noanswer;

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
        SolveRecursive(packed, occurrences, varcount, state);

    if (satisfiable) {
        for (int var = 1; var <= varcount; ++var) {
            result[var - 1] = state.assignment[var] == noanswer
                                       ? TRUE
                                       : state.assignment[var];
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
    DestroyOT(occurrences);
    DestroyPF(packed);
    return satisfiable ? TRUE : FALSE;
}
