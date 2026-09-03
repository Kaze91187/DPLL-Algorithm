#include "Global.h"

#include <limits>
#include <vector>

typedef Status (*SolverFunction)(Headnode*, int*, int);

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

static int SolveFile(
    const std::string& filename,
    bool verify,
    SolverFunction solver
) {
    int varnum = 0;
    Headnode* formula = Create(filename, varnum);

    if (verify) {
        std::cout << "CNF verification:\n";
        PrintFormula(formula);
    }

    int* result = new int[varnum];
    const std::clock_t start = std::clock();
    const Status status = solver(formula, result, varnum);
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

static std::string RemoveQuotes(const std::string& path) {
    if (path.size() >= 2 && path[0] == '"' && path[path.size() - 1] == '"')
        return path.substr(1, path.size() - 2);
    return path;
}

static int StartMenu(const std::string& programPath) {
    std::cout << "==============================\n"
              << " DPLL SAT and Star Sudoku\n"
              << "==============================\n"
              << "1. Game mode\n"
              << "2. Test mode\n"
              << "0. Exit\n"
              << "Select: ";

    int mode = -1;
    if (!(std::cin >> mode))
        return 2;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (mode == 0)
        return 0;

    if (mode == 1) {
        return Playstar(
            ProgramFilePath(programPath, "Asterisk-sudoku-levels.txt")
        );
    }

    if (mode != 2)
        throw std::runtime_error("Invalid mode");

    int filecount = 0;
    std::cout << "Number of CNF files: ";
    if (!(std::cin >> filecount) || filecount <= 0)
        throw std::runtime_error("Invalid file count");
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::vector<std::string> files(filecount);
    for (int i = 0; i < filecount; ++i) {
        std::cout << "CNF file " << i + 1 << ": ";
        std::getline(std::cin, files[i]);
        files[i] = RemoveQuotes(files[i]);
    }

    unsigned int timeoutMs = 30000;
    std::cout << "Time limit in milliseconds (0 = 30000): ";
    unsigned int entered = 0;
    if (std::cin >> entered && entered != 0)
        timeoutMs = entered;

    return Testmode(programPath, files.data(), filecount, timeoutMs);
}

int main(int argc, char* argv[]) {
    try {
        if (argc == 1)
            return StartMenu(argv[0]);

        if (argc == 3 && std::string(argv[1]) == "--worker-basic")
            return SolveFile(argv[2], false, DPLLBasic);

        if (argc == 3 && std::string(argv[1]) == "--worker-opt")
            return SolveFile(argv[2], false, DPLL);

        if (argc == 3 && std::string(argv[1]) == "--star")
            return Solvestar(argv[2]);

        if (argc == 2 && std::string(argv[1]) == "--game")
            return Playstar(
                ProgramFilePath(argv[0], "Asterisk-sudoku-levels.txt")
            );

        if (argc == 3 && std::string(argv[1]) == "--game")
            return Playstar(argv[2]);

        if (argc >= 3 && std::string(argv[1]) == "--test") {
            std::vector<std::string> files;
            for (int i = 2; i < argc; ++i)
                files.push_back(RemoveQuotes(argv[i]));
            return Testmode(argv[0], files.data(),
                            static_cast<int>(files.size()), 30000);
        }

        if (argc >= 4 && std::string(argv[1]) == "--test-limit") {
            char* end = nullptr;
            const unsigned long limit = std::strtoul(argv[2], &end, 10);
            if (end == argv[2] || *end != '\0' || limit == 0)
                throw std::runtime_error("Invalid test time limit");

            std::vector<std::string> files;
            for (int i = 3; i < argc; ++i)
                files.push_back(RemoveQuotes(argv[i]));
            return Testmode(argv[0], files.data(),
                            static_cast<int>(files.size()),
                            static_cast<unsigned int>(limit));
        }

        if (argc < 2 || argc > 3 ||
            (argc == 3 && std::string(argv[2]) != "--verify")) {
            std::cout << "Usage: rewrite_dpll <file.cnf> [--verify]\n"
                      << "       rewrite_dpll --star <puzzle.txt>\n"
                      << "       rewrite_dpll --game [puzzle.txt]\n"
                      << "       rewrite_dpll --test <a.cnf> [b.cnf ...]\n"
                      << "       rewrite_dpll --test-limit <ms> <a.cnf> ...\n";
            return 2;
        }

        return SolveFile(argv[1], argc == 3, DPLL);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
