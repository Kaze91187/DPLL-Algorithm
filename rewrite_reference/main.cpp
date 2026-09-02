#include "Global.h"

static std::string ResultPath(const std::string& cnfPath) {
    const std::size_t slash = cnfPath.find_last_of("/\\");
    std::size_t dot = cnfPath.find_last_of('.');

    if (dot == std::string::npos ||
        (slash != std::string::npos && dot < slash))
        dot = cnfPath.size();

    return cnfPath.substr(0, dot) + ".res";
}

static std::string ProgramFilePath(
    const std::string& programPath,
    const std::string& filename
) {
    const std::size_t slash = programPath.find_last_of("/\\");
    return slash == std::string::npos
               ? filename
               : programPath.substr(0, slash + 1) + filename;
}

static void PrintFormula(const Headnode* formula) {
    for (const Headnode* clause = formula; clause != nullptr; clause = clause->next) {
        for (const Datanode* node = clause->first; node != nullptr; node = node->next)
            std::cout << node->data << ' ';
        std::cout << "0\n";
    }
}

static int SolveFile(const std::string& filename, bool verify) {
    int varnum = 0;
    Headnode* formula = Create(filename, varnum);

    if (verify) {
        std::cout << "CNF verification:\n";
        PrintFormula(formula);
    }

    int* result = new int[varnum];
    const std::clock_t start = std::clock();
    const Status status = DPLL(formula, result, varnum);
    const std::clock_t finish = std::clock();
    const double milliseconds =
        static_cast<double>(finish - start) / CLOCKS_PER_SEC * 1000.0;

    const std::string outputPath = ResultPath(filename);
    std::ofstream output(outputPath);
    if (!output) {
        delete[] result;
        Destroy(formula);
        throw std::runtime_error("Cannot create result file: " + outputPath);
    }

    output << "s " << (status == TRUE ? 1 : 0) << '\n';
    output << "v";
    if (status == TRUE) {
        for (int i = 0; i < varnum; ++i)
            output << ' ' << (result[i] == TRUE ? i + 1 : -(i + 1));
    }
    output << " 0\n";
    output << "t " << milliseconds << '\n';

    std::cout << "Result: " << (status == TRUE ? "SAT" : "UNSAT")
              << "\nTime: " << milliseconds
              << " ms\nSaved: " << outputPath << '\n';

    delete[] result;
    Destroy(formula);
    return status == TRUE ? 0 : 1;
}

int main(int argc, char* argv[]) {
    try {
        if (argc == 1) {
            return Playstar(
                ProgramFilePath(argv[0], "Asterisk-sudoku-levels.txt")
            );
        }

        if (argc == 3 && std::string(argv[1]) == "--star")
            return Solvestar(argv[2]);

        if (argc == 3 && std::string(argv[1]) == "--game")
            return Playstar(argv[2]);

        if (argc < 2 || argc > 3 ||
            (argc == 3 && std::string(argv[2]) != "--verify")) {
            std::cout << "Usage: rewrite_dpll <file.cnf> [--verify]\n"
                      << "       rewrite_dpll --star <puzzle.txt>\n"
                      << "       rewrite_dpll --game <puzzle.txt>\n";
            return 2;
        }

        return SolveFile(argv[1], argc == 3);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
