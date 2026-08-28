#include "Global.h"

static string resultPath(const string& cnfPath) {
    size_t slash = cnfPath.find_last_of("/\\");
    size_t dot = cnfPath.find_last_of('.');
    if (dot == string::npos || (slash != string::npos && dot < slash))
        dot = cnfPath.size();
    return cnfPath.substr(0, dot) + ".res";
}

static void printFormula(HeadNode* formula) {
    for (HeadNode* clause = formula; clause != nullptr; clause = clause->down) {
        for (DataNode* literal = clause->right; literal != nullptr; literal = literal->next)
            cout << literal->data << ' ';
        cout << "0\n";
    }
}

static int solveFile(const string& filename, bool verify) {
    int variableCount = 0;
    string input = filename;
    HeadNode* formula = CreateClause(variableCount, input);
    if (verify) {
        cout << "CNF verification:\n";
        printFormula(formula);
    }

    consequence* result = new consequence[variableCount];
    clock_t start = clock();
    status value = DPLL(formula, result, variableCount);
    clock_t end = clock();

    string output = resultPath(filename);
    ofstream resultFile(output);
    if (!resultFile)
        throw runtime_error("Cannot create result file: " + output);
    resultFile << "s " << (value == TRUE ? 1 : 0) << '\n';
    resultFile << "v";
    if (value == TRUE) {
        for (int i = 0; i < variableCount; ++i)
            resultFile << ' ' << (result[i].value == TRUE ? i + 1 : -(i + 1));
    }
    resultFile << " 0\n";
    resultFile << "t " << (double)(end - start) / CLOCKS_PER_SEC * 1000.0 << '\n';
    cout << "Result: " << (value == TRUE ? "SAT" : "UNSAT")
         << "\nTime: " << (double)(end - start) / CLOCKS_PER_SEC * 1000.0
         << " ms\nSaved: " << output << '\n';
    DestroyFormula(formula);
    delete[] result;
    return value == TRUE ? 0 : 1;
}

int main(int argc, char* argv[]) {
    try {
        if (argc == 3 && string(argv[1]) == "--star")
            return solveStarSudoku(argv[2]);
        if (argc < 2 || argc > 3) {
            cout << "Usage: dpll <file.cnf> [--verify]\n"
                 << "       dpll --star <puzzle.txt>\n";
            return 0;
        }
        if (argc == 3 && string(argv[2]) != "--verify") {
            cout << "Usage: dpll <file.cnf> [--verify]\n"
                 << "       dpll --star <puzzle.txt>\n";
            return 2;
        }
        return solveFile(argv[1], argc == 3);
    } catch (const exception& error) {
        cerr << error.what() << '\n';
        return 2;
    }
}
