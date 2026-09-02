#include "Global.h"

// row、column、digit 均从 0 开始，返回连续编号 1~729。
static int SudokuVariable(int row, int column, int digit) {
    return (row * 9 + column) * 9 + digit + 1;
}

static void AddClause(
    Headnode*& formula,
    Headnode*& tail,
    const int* literals,
    int count
) {
    Headnode* clause = new Headnode;
    Datanode* literalTail = nullptr;

    for (int i = 0; i < count; ++i) {
        Datanode* node = new Datanode;
        node->data = literals[i];

        if (clause->first == nullptr)
            clause->first = node;
        else
            literalTail->next = node;

        literalTail = node;
        ++clause->num;
    }

    if (formula == nullptr)
        formula = clause;
    else
        tail->next = clause;
    tail = clause;
}

static void AddCellConstraints(
    Headnode*& formula,
    Headnode*& tail,
    int row,
    int column
) {
    int candidates[9];
    for (int digit = 0; digit < 9; ++digit)
        candidates[digit] = SudokuVariable(row, column, digit);

    // 该格至少选择一个数字。
    AddClause(formula, tail, candidates, 9);

    // 该格任意两个数字不能同时为真，即该格至多选择一个数字。
    for (int first = 0; first < 9; ++first) {
        for (int second = first + 1; second < 9; ++second) {
            const int pair[2] = {-candidates[first], -candidates[second]};
            AddClause(formula, tail, pair, 2);
        }
    }
}

static void AddRegionConstraints(
    Headnode*& formula,
    Headnode*& tail,
    const int cells[9][2]
) {
    for (int digit = 0; digit < 9; ++digit) {
        int positions[9];
        for (int i = 0; i < 9; ++i)
            positions[i] = SudokuVariable(cells[i][0], cells[i][1], digit);

        // 区域内该数字至少出现一次。
        AddClause(formula, tail, positions, 9);

        // 区域内该数字不能出现在两个不同格子中。
        for (int first = 0; first < 9; ++first) {
            for (int second = first + 1; second < 9; ++second) {
                const int pair[2] = {-positions[first], -positions[second]};
                AddClause(formula, tail, pair, 2);
            }
        }
    }
}

static Headnode* BuildStarFormula(const std::string& puzzle) {
    Headnode* formula = nullptr;
    Headnode* tail = nullptr;

    // 1. 每格恰好填一个数，并加入题目提示数。
    for (int row = 0; row < 9; ++row) {
        for (int column = 0; column < 9; ++column) {
            AddCellConstraints(formula, tail, row, column);

            const char given = puzzle[row * 9 + column];
            if (given >= '1' && given <= '9') {
                const int literal = SudokuVariable(row, column, given - '1');
                AddClause(formula, tail, &literal, 1);
            }
        }
    }

    // 2. 九行约束。
    for (int row = 0; row < 9; ++row) {
        int cells[9][2];
        for (int column = 0; column < 9; ++column) {
            cells[column][0] = row;
            cells[column][1] = column;
        }
        AddRegionConstraints(formula, tail, cells);
    }

    // 3. 九列约束。
    for (int column = 0; column < 9; ++column) {
        int cells[9][2];
        for (int row = 0; row < 9; ++row) {
            cells[row][0] = row;
            cells[row][1] = column;
        }
        AddRegionConstraints(formula, tail, cells);
    }

    // 4. 九个 3x3 宫约束。
    for (int boxRow = 0; boxRow < 3; ++boxRow) {
        for (int boxColumn = 0; boxColumn < 3; ++boxColumn) {
            int cells[9][2];
            int index = 0;
            for (int row = 0; row < 3; ++row) {
                for (int column = 0; column < 3; ++column) {
                    cells[index][0] = boxRow * 3 + row;
                    cells[index][1] = boxColumn * 3 + column;
                    ++index;
                }
            }
            AddRegionConstraints(formula, tail, cells);
        }
    }

    // 5. 任务书规定的九个星形格（此处坐标从 0 开始）。
    const int starCells[9][2] = {
        {1, 4}, {2, 2}, {2, 6},
        {4, 1}, {4, 4}, {4, 7},
        {6, 2}, {6, 6}, {7, 4}
    };
    AddRegionConstraints(formula, tail, starCells);

    return formula;
}

Status SolveStarPuzzle(const std::string& puzzle, int answer[ROW][COL]) {
    if (puzzle.size() < ROW * COL)
        throw std::runtime_error("Star sudoku puzzle must contain 81 cells");

    Headnode* formula = BuildStarFormula(puzzle);
    int result[729];
    const Status status = DPLL(formula, result, 729);

    if (status == TRUE) {
        for (int row = 0; row < ROW; ++row) {
            for (int column = 0; column < COL; ++column) {
                answer[row][column] = 0;
                for (int digit = 0; digit < 9; ++digit) {
                    if (result[SudokuVariable(row, column, digit) - 1] == TRUE) {
                        answer[row][column] = digit + 1;
                        break;
                    }
                }
            }
        }
    }

    Destroy(formula);
    return status;
}

int Solvestar(const std::string& filename) {
    std::ifstream input(filename);
    if (!input)
        throw std::runtime_error("Cannot open star sudoku file: " + filename);

    std::string puzzle;
    while (std::getline(input, puzzle)) {
        if (puzzle.size() >= 81 && puzzle[0] != '/')
            break;
    }
    if (puzzle.size() < 81)
        throw std::runtime_error("No 81-character puzzle was found");

    int answer[ROW][COL];

    const std::clock_t start = std::clock();
    const Status status = SolveStarPuzzle(puzzle, answer);
    const std::clock_t finish = std::clock();

    if (status == TRUE) {
        std::cout << "Star sudoku solution:\n";
        for (int row = 0; row < 9; ++row) {
            for (int column = 0; column < 9; ++column) {
                std::cout << answer[row][column]
                          << (column == 8 ? '\n' : ' ');
            }
        }
    } else {
        std::cout << "Star sudoku has no solution.\n";
    }

    std::cout << "Time: "
              << static_cast<double>(finish - start) / CLOCKS_PER_SEC * 1000.0
              << " ms\n";
    return status == TRUE ? 0 : 1;
}
