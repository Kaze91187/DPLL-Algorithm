#include "Global.h"

#include <iomanip>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

struct ResData {
    int status = -1;
    double time = -1.0;
    bool valid = false;
};

struct TestData {
    std::string name;
    int vars = 0;
    int clauses = 0;
    int status = -1;
    double basicTime = -1.0;
    double optimizedTime = -1.0;
    bool basicTimeout = false;
    bool optimizedTimeout = false;
};

std::string ResultPath(const std::string& cnfPath) {
    const std::size_t slash = cnfPath.find_last_of("/\\");
    std::size_t dot = cnfPath.find_last_of('.');
    if (dot == std::string::npos ||
        (slash != std::string::npos && dot < slash)) {
        dot = cnfPath.size();
    }
    return cnfPath.substr(0, dot) + ".res";
}

std::string FileName(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

bool ReadHeader(const std::string& filename, int& vars, int& clauses) {
    std::ifstream input(filename);
    std::string line;
    while (std::getline(input, line)) {
        const std::size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == 'c')
            continue;
        std::istringstream stream(line.substr(first));
        std::string marker;
        std::string format;
        stream >> marker;
        if (marker == "p") {
            stream >> format >> vars >> clauses;
            return format == "cnf" && vars > 0 && clauses >= 0;
        }
    }
    return false;
}

ResData ReadResult(const std::string& filename) {
    ResData result;
    std::ifstream input(filename);
    if (!input)
        return result;

    std::string marker;
    while (input >> marker) {
        if (marker == "s") {
            input >> result.status;
        } else if (marker == "t") {
            input >> result.time;
        } else {
            std::string rest;
            std::getline(input, rest);
        }
    }
    result.valid = result.status >= -1 && result.status <= 1 && result.time >= 0;
    return result;
}

void WriteUnknownResult(const std::string& filename, double time) {
    std::ofstream output(filename);
    if (!output)
        throw std::runtime_error("Cannot create result file: " + filename);
    output << "s -1\n";
    output << "v 0\n";
    output << "t " << time << '\n';
}

#ifdef _WIN32

std::string FullPath(const std::string& path) {
    char buffer[MAX_PATH];
    const DWORD length = GetFullPathNameA(path.c_str(), MAX_PATH, buffer, nullptr);
    return length > 0 && length < MAX_PATH ? std::string(buffer) : path;
}

std::string Quote(const std::string& text) {
    return "\"" + text + "\"";
}

bool RunWorker(
    const std::string& program,
    const std::string& mode,
    const std::string& cnf,
    unsigned int timeoutMs,
    bool& timedOut
) {
    std::string command = Quote(program) + " " + mode + " " + Quote(cnf);
    std::vector<char> commandLine(command.begin(), command.end());
    commandLine.push_back('\0');

    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process = {};

    if (!CreateProcessA(nullptr, commandLine.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process)) {
        return false;
    }

    const DWORD wait = WaitForSingleObject(process.hProcess, timeoutMs);
    timedOut = wait == WAIT_TIMEOUT;
    if (timedOut) {
        TerminateProcess(process.hProcess, 3);
        WaitForSingleObject(process.hProcess, 5000);
    }

    DWORD exitCode = 3;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return !timedOut && exitCode <= 2;
}

#else

bool RunWorker(const std::string&, const std::string&, const std::string&,
               unsigned int, bool& timedOut) {
    timedOut = false;
    return false;
}

std::string FullPath(const std::string& path) {
    return path;
}

#endif

const char* StatusText(int status) {
    if (status == 1)
        return "SAT";
    if (status == 0)
        return "UNSAT";
    return "UNKNOWN";
}

void PrintTable(const std::vector<TestData>& tests) {
    std::cout << '\n'
              << std::left << std::setw(31) << "Case"
              << std::right << std::setw(8) << "Vars"
              << std::setw(10) << "Clauses"
              << std::setw(10) << "C/V"
              << std::setw(11) << "Result"
              << std::setw(13) << "t(ms)"
              << std::setw(13) << "to(ms)"
              << std::setw(12) << "Rate"
              << '\n';
    std::cout << std::string(108, '-') << '\n';

    for (std::size_t i = 0; i < tests.size(); ++i) {
        const TestData& test = tests[i];
        const double ratio = test.vars == 0
                                 ? 0.0
                                 : static_cast<double>(test.clauses) / test.vars;

        std::cout << std::left << std::setw(31) << test.name.substr(0, 30)
                  << std::right << std::setw(8) << test.vars
                  << std::setw(10) << test.clauses
                  << std::setw(10) << std::fixed << std::setprecision(2) << ratio
                  << std::setw(11) << StatusText(test.status);

        if (test.basicTimeout)
            std::cout << std::setw(13) << "TIMEOUT";
        else
            std::cout << std::setw(13) << std::fixed << std::setprecision(2)
                      << test.basicTime;

        if (test.optimizedTimeout)
            std::cout << std::setw(13) << "TIMEOUT";
        else
            std::cout << std::setw(13) << std::fixed << std::setprecision(2)
                      << test.optimizedTime;

        if (!test.basicTimeout && !test.optimizedTimeout && test.basicTime > 0) {
            const double rate =
                (test.basicTime - test.optimizedTime) / test.basicTime * 100.0;
            std::ostringstream rateText;
            rateText << std::fixed << std::setprecision(2) << rate << '%';
            std::cout << std::setw(12) << rateText.str();
        } else {
            std::cout << std::setw(12) << "--";
        }
        std::cout << '\n';
    }
}

} // namespace

int Testmode(
    const std::string& programPath,
    const std::string* files,
    int filecount,
    unsigned int timeoutMs
) {
    if (filecount <= 0)
        throw std::runtime_error("No CNF test case was selected");

    const std::string program = FullPath(programPath);
    std::vector<TestData> tests;

    std::cout << "Time limit: " << timeoutMs << " ms per solver per case.\n";
    for (int i = 0; i < filecount; ++i) {
        TestData test;
        const std::string cnf = FullPath(files[i]);
        test.name = FileName(cnf);
        if (!ReadHeader(cnf, test.vars, test.clauses)) {
            std::cerr << "Invalid CNF header: " << cnf << '\n';
            test.optimizedTimeout = true;
            tests.push_back(test);
            continue;
        }

        const std::string res = ResultPath(cnf);
#ifdef _WIN32
        DeleteFileA(res.c_str());
#endif
        bool basicTimeout = false;
        const bool basicRan = RunWorker(program, "--worker-basic", cnf,
                                        timeoutMs, basicTimeout);
        test.basicTimeout = basicTimeout || !basicRan;
        if (!test.basicTimeout) {
            const ResData basic = ReadResult(res);
            if (basic.valid)
                test.basicTime = basic.time;
            else
                test.basicTimeout = true;
        }

#ifdef _WIN32
        DeleteFileA(res.c_str());
#endif
        bool optimizedTimeout = false;
        const bool optimizedRan = RunWorker(program, "--worker-opt", cnf,
                                            timeoutMs, optimizedTimeout);
        test.optimizedTimeout = optimizedTimeout || !optimizedRan;
        if (!test.optimizedTimeout) {
            const ResData optimized = ReadResult(res);
            if (optimized.valid) {
                test.status = optimized.status;
                test.optimizedTime = optimized.time;
            } else {
                test.optimizedTimeout = true;
            }
        }

        if (test.optimizedTimeout) {
            test.status = -1;
            test.optimizedTime = timeoutMs;
            WriteUnknownResult(res, timeoutMs);
        }
        tests.push_back(test);
        std::cout << "Finished " << test.name << "\n";
    }

    PrintTable(tests);
    return 0;
}
