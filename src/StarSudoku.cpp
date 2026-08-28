#include "Global.h"

static int variable(int row, int column, int digit) {
    return (row * 9 + column) * 9 + digit + 1;
}

static void addClause(HeadNode*& formula, HeadNode*& tail, const int* literals, int count) {
    HeadNode* clause = new HeadNode;
    DataNode* dataTail = nullptr;
    for (int i = 0; i < count; ++i) {
        DataNode* data = new DataNode;
        data->data = literals[i];
        if (dataTail == nullptr)
            clause->right = data;
        else
            dataTail->next = data;
        dataTail = data;
        clause->Num++;
    }
    if (formula == nullptr)
        formula = clause;
    else
        tail->down = clause;
    tail = clause;
}

static void addAtLeastOne(HeadNode*& formula, HeadNode*& tail, int row, int column) {
    int literals[9];
    for (int digit = 0; digit < 9; ++digit)
        literals[digit] = variable(row, column, digit);
    addClause(formula, tail, literals, 9);
}

static void addDifferent(HeadNode*& formula, HeadNode*& tail, int first, int second) {
    int literals[2] = {-first, -second};
    addClause(formula, tail, literals, 2);
}

static void addRegionConstraints(HeadNode*& formula, HeadNode*& tail, const int cells[][2], int cellCount) {
    for (int digit = 0; digit < 9; ++digit) {
        int literals[9];
        for (int i = 0; i < cellCount; ++i)
            literals[i] = variable(cells[i][0], cells[i][1], digit);
        addClause(formula, tail, literals, cellCount);
        for (int i = 0; i < cellCount; ++i)
            for (int j = i + 1; j < cellCount; ++j)
                addDifferent(formula, tail, literals[i], literals[j]);
    }
}

static HeadNode* buildStarFormula(const string& puzzle) {
    HeadNode* formula = nullptr;
    HeadNode* tail = nullptr;
    for (int row = 0; row < 9; ++row)
        for (int column = 0; column < 9; ++column) {
            addAtLeastOne(formula, tail, row, column);
            for (int first = 0; first < 9; ++first)
                for (int second = first + 1; second < 9; ++second)
                    addDifferent(formula, tail, variable(row, column, first),
                                 variable(row, column, second));
            char value = puzzle[row * 9 + column];
            if (value >= '1' && value <= '9') {
                int literal = variable(row, column, value - '1');
                addClause(formula, tail, &literal, 1);
            }
        }

    for (int row = 0; row < 9; ++row) {
        int cells[9][2];
        for (int column = 0; column < 9; ++column) {
            cells[column][0] = row;
            cells[column][1] = column;
        }
        addRegionConstraints(formula, tail, cells, 9);
    }
    for (int column = 0; column < 9; ++column) {
        int cells[9][2];
        for (int row = 0; row < 9; ++row) {
            cells[row][0] = row;
            cells[row][1] = column;
        }
        addRegionConstraints(formula, tail, cells, 9);
    }
    for (int boxRow = 0; boxRow < 3; ++boxRow)
        for (int boxColumn = 0; boxColumn < 3; ++boxColumn) {
            int cells[9][2];
            int index = 0;
            for (int row = 0; row < 3; ++row)
                for (int column = 0; column < 3; ++column) {
                    cells[index][0] = boxRow * 3 + row;
                    cells[index++][1] = boxColumn * 3 + column;
                }
            addRegionConstraints(formula, tail, cells, 9);
        }

    const int starCells[9][2] = {{1, 4}, {2, 2}, {2, 6}, {4, 1}, {4, 4},
                                 {4, 7}, {6, 2}, {6, 6}, {8, 4}};
    addRegionConstraints(formula, tail, starCells, 9);
    return formula;
}

int solveStarSudoku(const string& filename) {
    ifstream input(filename);
    if (!input)
        throw runtime_error("Cannot open star sudoku file: " + filename);
    string puzzle;
    while (getline(input, puzzle)) {
        if (puzzle.size() >= 81 && puzzle[0] != '/')
            break;
    }
    if (puzzle.size() < 81)
        throw runtime_error("Star sudoku file does not contain an 81-character puzzle");

    HeadNode* formula = buildStarFormula(puzzle);
    consequence result[729];
    clock_t start = clock();
    status value = DPLL(formula, result, 729);
    clock_t end = clock();
    if (value == TRUE) {
        cout << "Star sudoku solution:\n";
        for (int row = 0; row < 9; ++row) {
            for (int column = 0; column < 9; ++column) {
                int digit = 0;
                for (int candidate = 0; candidate < 9; ++candidate)
                    if (result[variable(row, column, candidate) - 1].value == TRUE)
                        digit = candidate + 1;
                cout << digit << (column == 8 ? '\n' : ' ');
            }
        }
    } else {
        cout << "Star sudoku has no solution.\n";
    }
    cout << "Time: " << (double)(end - start) / CLOCKS_PER_SEC * 1000.0 << " ms\n";
    DestroyFormula(formula);
    return value == TRUE ? 0 : 1;
}
