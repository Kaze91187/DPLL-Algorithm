#include "Global.h"

Headnode* ParseCnf(const std::string& filename, int& variableCount) {
    std::ifstream input(filename);
    if (!input)
        throw std::runtime_error("Cannot open CNF file: " + filename);

    std::string line;
    std::string marker;
    std::string format;
    int declaredVariables = 0;
    int declaredClauses = -1;

    // 先跳过注释和空行，找到 p cnf <变量数> <子句数>。
    while (std::getline(input, line)) {
        const std::size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == 'c')
            continue;

        std::istringstream header(line.substr(first));
        header >> marker;
        if (marker == "p") {
            header >> format >> declaredVariables >> declaredClauses;
            break;
        }
    }

    if (format != "cnf" || declaredVariables <= 0 || declaredClauses < 0)
        throw std::runtime_error("Invalid DIMACS header: " + filename);

    Headnode* formula = nullptr;
    Headnode* formulaTail = nullptr;
    Headnode* currentClause = new Headnode;
    Datanode* literalTail = nullptr;
    int parsedClauses = 0;

    try {
        while (std::getline(input, line)) {
            const std::size_t first = line.find_first_not_of(" \t\r");
            if (first == std::string::npos || line[first] == 'c')
                continue;

            std::istringstream numbers(line.substr(first));
            int literal = 0;
            while (numbers >> literal) {
                if (literal == 0) {
                    if (formula == nullptr)
                        formula = currentClause;
                    else
                        formulaTail->next = currentClause;

                    formulaTail = currentClause;
                    currentClause = new Headnode;
                    literalTail = nullptr;
                    ++parsedClauses;
                    continue;
                }

                if (std::abs(literal) > declaredVariables)
                    throw std::runtime_error("Literal exceeds declared variable range");

                Datanode* node = new Datanode;
                node->data = literal;

                if (currentClause->first == nullptr)
                    currentClause->first = node;
                else
                    literalTail->next = node;

                literalTail = node;
                ++currentClause->num;
            }

            if (!numbers.eof())
                throw std::runtime_error("Invalid token in CNF file");
        }

        // 文件结束时 currentClause 应当是读完最后一个 0 后创建的空闲结点。
        if (currentClause->num != 0)
            throw std::runtime_error("Last clause is missing terminating 0");

        delete currentClause;
        currentClause = nullptr;

        if (parsedClauses != declaredClauses)
            throw std::runtime_error("DIMACS clause count mismatch");
    } catch (...) {
        if (currentClause != nullptr) {
            Datanode* literal = currentClause->first;
            while (literal != nullptr) {
                Datanode* nextLiteral = literal->next;
                delete literal;
                literal = nextLiteral;
            }
            delete currentClause;
        }
        DestroyFormula(formula);
        throw;
    }

    variableCount = declaredVariables;
    std::cout << "Parsed " << declaredVariables << " variables and "
              << declaredClauses << " clauses.\n";
    return formula;
}
