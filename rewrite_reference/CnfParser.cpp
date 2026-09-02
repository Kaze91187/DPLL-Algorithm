#include "Global.h"

using namespace std;

Headnode* Create(const string& filename, int& varnum) {
    ifstream input(filename);
    if (!input)
        throw runtime_error("Cannot open CNF file: " + filename);

    string line;
    string marker;
    string format;
    int filevarnum = 0;   // 文件头中声明的变量数
    int clanum = -1;      // 任务书中声明的子句数

    // 先跳过注释和空行，找到 p cnf <变量数> <子句数>。
    while (getline(input, line)) {
        const size_t first = line.find_first_not_of(" \t\r");
        if (first == string::npos || line[first] == 'c')
            continue;

        istringstream header(line.substr(first));
        header >> marker;
        if (marker == "p") {
            header >> format >> filevarnum >> clanum;
            break;
        }
    }

    if (format != "cnf" || filevarnum <= 0 || clanum < 0)
        throw runtime_error("Invalid DIMACS header: " + filename);

    Headnode* formula = nullptr;
    Headnode* ftail = nullptr;       // 最后一个子句
    Headnode* curcla = new Headnode; // 当前子句
    Datanode* ltail = nullptr;       // 当前子句的最后一个文字
    int clausenum = 0;               // 实际解析出的子句数

    try {
        while (getline(input, line)) {
            const size_t first = line.find_first_not_of(" \t\r");
            if (first == string::npos || line[first] == 'c')
                continue;

            istringstream numbers(line.substr(first));
            int literal = 0;
            while (numbers >> literal) {
                if (literal == 0) {
                    if (formula == nullptr)
                        formula = curcla;
                    else
                        ftail->next = curcla;

                    ftail = curcla;
                    curcla = new Headnode;
                    ltail = nullptr;
                    ++clausenum;
                    continue;
                }

                if (abs(literal) > filevarnum)
                    throw runtime_error("Literal exceeds declared variable range");

                Datanode* node = new Datanode;
                node->data = literal;

                if (curcla->first == nullptr)
                    curcla->first = node;
                else
                    ltail->next = node;

                ltail = node;
                ++curcla->num;
            }

            if (!numbers.eof())
                throw runtime_error("Invalid token in CNF file");
        }

        // 文件结束时，curcla 应是读完最后一个 0 后创建的空闲结点。
        if (curcla->num != 0)
            throw runtime_error("Last clause is missing terminating 0");

        delete curcla;
        curcla = nullptr;

        if (clausenum != clanum)
            throw runtime_error("DIMACS clause count mismatch");
    } catch (...) {
        if (curcla != nullptr) {
            Datanode* literal = curcla->first;
            while (literal != nullptr) {
                Datanode* nextl = literal->next;
                delete literal;
                literal = nextl;
            }
            delete curcla;
        }
        Destroy(formula);
        throw;
    }

    varnum = filevarnum;
    cout << "Parsed " << filevarnum << " variables and "
         << clanum << " clauses.\n";
    return formula;
}
