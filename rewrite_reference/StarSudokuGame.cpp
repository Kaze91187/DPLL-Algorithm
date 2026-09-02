#include "Global.h"

#include <vector>

#ifdef _WIN32

#include <windows.h>
#include <windowsx.h>

namespace {

const int GRID_X = 30;
const int GRID_Y = 70;
const int CELL_SIZE = 52;
const int GRID_SIZE = CELL_SIZE * 9;
const int SIDE_X = 530;
const int BUTTON_WIDTH = 145;
const int BUTTON_HEIGHT = 42;
const int PAD_SIZE = 45;

struct GameState {
    std::vector<std::string> levels;
    int level = 0;
    int board[ROW][COL] = {};
    int original[ROW][COL] = {};
    int solution[ROW][COL] = {};
    bool fixed[ROW][COL] = {};
    int selectedRow = -1;
    int selectedColumn = -1;
    bool keypadVisible = false;
    std::wstring message = L"点击空格开始填写";
};

RECT MakeRect(int left, int top, int width, int height) {
    RECT rect = {left, top, left + width, top + height};
    return rect;
}

RECT PreviousButton() {
    return MakeRect(SIDE_X, 82, BUTTON_WIDTH, BUTTON_HEIGHT);
}

RECT NextButton() {
    return MakeRect(SIDE_X, 134, BUTTON_WIDTH, BUTTON_HEIGHT);
}

RECT CheckButton() {
    return MakeRect(SIDE_X, 204, BUTTON_WIDTH, BUTTON_HEIGHT);
}

RECT SolveButton() {
    return MakeRect(SIDE_X, 256, BUTTON_WIDTH, BUTTON_HEIGHT);
}

RECT ResetButton() {
    return MakeRect(SIDE_X, 308, BUTTON_WIDTH, BUTTON_HEIGHT);
}

RECT KeypadRect() {
    return MakeRect(SIDE_X, 398, PAD_SIZE * 3, PAD_SIZE * 3);
}

bool IsPuzzleLine(const std::string& line) {
    if (line.size() < ROW * COL)
        return false;

    for (int i = 0; i < ROW * COL; ++i) {
        const char cell = line[i];
        if (cell != '.' && (cell < '1' || cell > '9'))
            return false;
    }
    return true;
}

std::vector<std::string> LoadLevels(const std::string& filename) {
    std::ifstream input(filename);
    if (!input)
        throw std::runtime_error("Cannot open star sudoku file: " + filename);

    std::vector<std::string> levels;
    std::string line;
    while (std::getline(input, line)) {
        if (IsPuzzleLine(line))
            levels.push_back(line.substr(0, ROW * COL));
    }

    if (levels.empty())
        throw std::runtime_error("No valid 81-character puzzle was found");
    return levels;
}

bool LoadLevel(GameState& game, int level) {
    int answer[ROW][COL];
    if (SolveStarPuzzle(game.levels[level], answer) != TRUE)
        return false;

    game.level = level;
    const std::string& puzzle = game.levels[level];
    for (int row = 0; row < ROW; ++row) {
        for (int column = 0; column < COL; ++column) {
            const char cell = puzzle[row * COL + column];
            const int value = cell >= '1' && cell <= '9' ? cell - '0' : 0;
            game.board[row][column] = value;
            game.original[row][column] = value;
            game.solution[row][column] = answer[row][column];
            game.fixed[row][column] = value != 0;
        }
    }

    game.selectedRow = -1;
    game.selectedColumn = -1;
    game.keypadVisible = false;
    game.message = L"点击空格开始填写";
    return true;
}

bool IsStarCell(int row, int column) {
    const int cells[9][2] = {
        {1, 4}, {2, 2}, {2, 6},
        {4, 1}, {4, 4}, {4, 7},
        {6, 2}, {6, 6}, {7, 4}
    };
    for (int i = 0; i < 9; ++i) {
        if (cells[i][0] == row && cells[i][1] == column)
            return true;
    }
    return false;
}

bool IsFilled(const GameState& game) {
    for (int row = 0; row < ROW; ++row) {
        for (int column = 0; column < COL; ++column) {
            if (game.board[row][column] == 0)
                return false;
        }
    }
    return true;
}

bool ValidGroup(const int values[9]) {
    bool used[10] = {};
    for (int i = 0; i < 9; ++i) {
        const int value = values[i];
        if (value < 1 || value > 9 || used[value])
            return false;
        used[value] = true;
    }
    return true;
}

bool IsCorrect(const GameState& game) {
    int values[9];

    for (int row = 0; row < ROW; ++row) {
        for (int column = 0; column < COL; ++column)
            values[column] = game.board[row][column];
        if (!ValidGroup(values))
            return false;
    }

    for (int column = 0; column < COL; ++column) {
        for (int row = 0; row < ROW; ++row)
            values[row] = game.board[row][column];
        if (!ValidGroup(values))
            return false;
    }

    for (int boxRow = 0; boxRow < 3; ++boxRow) {
        for (int boxColumn = 0; boxColumn < 3; ++boxColumn) {
            int idx = 0;
            for (int row = 0; row < 3; ++row) {
                for (int column = 0; column < 3; ++column)
                    values[idx++] = game.board[boxRow * 3 + row]
                                                [boxColumn * 3 + column];
            }
            if (!ValidGroup(values))
                return false;
        }
    }

    const int starCells[9][2] = {
        {1, 4}, {2, 2}, {2, 6},
        {4, 1}, {4, 4}, {4, 7},
        {6, 2}, {6, 6}, {7, 4}
    };
    for (int i = 0; i < 9; ++i)
        values[i] = game.board[starCells[i][0]][starCells[i][1]];
    return ValidGroup(values);
}

void DrawTextCentered(HDC dc, const wchar_t* text, const RECT& rect, COLORREF color) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    RECT copy = rect;
    DrawTextW(dc, text, -1, &copy,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void DrawButton(HDC dc, const RECT& rect, const wchar_t* text) {
    HBRUSH brush = CreateSolidBrush(RGB(236, 240, 245));
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
    FrameRect(dc, &rect, static_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
    DrawTextCentered(dc, text, rect, RGB(35, 48, 68));
}

void DrawGame(HWND window, HDC dc, const GameState& game) {
    RECT client;
    GetClientRect(window, &client);
    HBRUSH background = CreateSolidBrush(RGB(250, 251, 253));
    FillRect(dc, &client, background);
    DeleteObject(background);

    HFONT titleFont = CreateFontW(
        28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei"
    );
    HFONT normalFont = CreateFontW(
        22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei"
    );
    HFONT numberFont = CreateFontW(
        30, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei"
    );

    HFONT oldFont = static_cast<HFONT>(SelectObject(dc, titleFont));
    DrawTextCentered(dc, L"星形数独", MakeRect(30, 18, 468, 40), RGB(25, 48, 82));

    SelectObject(dc, normalFont);
    wchar_t levelText[64];
    wsprintfW(levelText, L"关卡 %d / %d", game.level + 1,
              static_cast<int>(game.levels.size()));
    DrawTextCentered(dc, levelText, MakeRect(SIDE_X, 25, BUTTON_WIDTH, 36),
                     RGB(25, 48, 82));

    SelectObject(dc, numberFont);
    for (int row = 0; row < ROW; ++row) {
        for (int column = 0; column < COL; ++column) {
            RECT cell = MakeRect(GRID_X + column * CELL_SIZE,
                                 GRID_Y + row * CELL_SIZE,
                                 CELL_SIZE, CELL_SIZE);
            COLORREF fill = RGB(255, 255, 255);
            if (IsStarCell(row, column))
                fill = RGB(255, 245, 191);
            if (game.fixed[row][column])
                fill = IsStarCell(row, column)
                           ? RGB(241, 223, 155)
                           : RGB(231, 235, 240);
            if (row == game.selectedRow && column == game.selectedColumn)
                fill = RGB(187, 222, 251);

            HBRUSH cellBrush = CreateSolidBrush(fill);
            FillRect(dc, &cell, cellBrush);
            DeleteObject(cellBrush);
            FrameRect(dc, &cell, static_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH)));

            if (game.board[row][column] != 0) {
                wchar_t digit[2] = {
                    static_cast<wchar_t>(L'0' + game.board[row][column]), 0
                };
                const COLORREF color = game.fixed[row][column]
                                           ? RGB(28, 39, 58)
                                           : RGB(25, 103, 170);
                DrawTextCentered(dc, digit, cell, color);
            }
        }
    }

    HPEN thickPen = CreatePen(PS_SOLID, 3, RGB(35, 48, 68));
    HPEN oldPen = static_cast<HPEN>(SelectObject(dc, thickPen));
    for (int i = 0; i <= 9; i += 3) {
        MoveToEx(dc, GRID_X + i * CELL_SIZE, GRID_Y, nullptr);
        LineTo(dc, GRID_X + i * CELL_SIZE, GRID_Y + GRID_SIZE);
        MoveToEx(dc, GRID_X, GRID_Y + i * CELL_SIZE, nullptr);
        LineTo(dc, GRID_X + GRID_SIZE, GRID_Y + i * CELL_SIZE);
    }
    SelectObject(dc, oldPen);
    DeleteObject(thickPen);

    SelectObject(dc, normalFont);
    DrawButton(dc, PreviousButton(), L"上一关");
    DrawButton(dc, NextButton(), L"下一关");
    DrawButton(dc, CheckButton(), L"检查答案");
    DrawButton(dc, SolveButton(), L"直接解析");
    DrawButton(dc, ResetButton(), L"重新开始");

    DrawTextCentered(dc, game.message.c_str(),
                     MakeRect(20, 550, 665, 32), RGB(62, 75, 93));

    if (game.keypadVisible) {
        DrawTextCentered(dc, L"选择数字", MakeRect(SIDE_X, 362, 135, 30),
                         RGB(35, 48, 68));
        RECT pad = KeypadRect();
        HBRUSH padBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(dc, &pad, padBrush);
        DeleteObject(padBrush);

        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                RECT key = MakeRect(SIDE_X + column * PAD_SIZE,
                                    398 + row * PAD_SIZE,
                                    PAD_SIZE, PAD_SIZE);
                FrameRect(dc, &key,
                          static_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
                wchar_t digit[2] = {
                    static_cast<wchar_t>(L'1' + row * 3 + column), 0
                };
                DrawTextCentered(dc, digit, key, RGB(25, 103, 170));
            }
        }
    }

    SelectObject(dc, oldFont);
    DeleteObject(numberFont);
    DeleteObject(normalFont);
    DeleteObject(titleFont);
}

bool PointIn(const RECT& rect, int x, int y) {
    POINT point = {x, y};
    return PtInRect(&rect, point) != FALSE;
}

void ChangeLevel(HWND window, GameState& game, int step) {
    const int count = static_cast<int>(game.levels.size());
    int next = (game.level + step + count) % count;
    if (!LoadLevel(game, next)) {
        MessageBoxW(window, L"这一关的题目无解。", L"关卡错误",
                    MB_OK | MB_ICONERROR);
    }
}

void ResetLevel(GameState& game) {
    for (int row = 0; row < ROW; ++row) {
        for (int column = 0; column < COL; ++column)
            game.board[row][column] = game.original[row][column];
    }
    game.selectedRow = -1;
    game.selectedColumn = -1;
    game.keypadVisible = false;
    game.message = L"本关已重置";
}

void HandleClick(HWND window, GameState& game, int x, int y) {
    if (x >= GRID_X && x < GRID_X + GRID_SIZE &&
        y >= GRID_Y && y < GRID_Y + GRID_SIZE) {
        const int column = (x - GRID_X) / CELL_SIZE;
        const int row = (y - GRID_Y) / CELL_SIZE;
        if (!game.fixed[row][column]) {
            game.selectedRow = row;
            game.selectedColumn = column;
            game.keypadVisible = true;
            game.message = L"请在右侧小键盘选择 1～9";
        } else {
            game.keypadVisible = false;
            game.message = L"提示数字不能修改";
        }
        InvalidateRect(window, nullptr, FALSE);
        return;
    }

    if (PointIn(PreviousButton(), x, y)) {
        ChangeLevel(window, game, -1);
    } else if (PointIn(NextButton(), x, y)) {
        ChangeLevel(window, game, 1);
    } else if (PointIn(CheckButton(), x, y)) {
        if (!IsFilled(game)) {
            game.message = L"还有空格未填写，可以继续填写或直接解析";
            MessageBoxW(window, L"题目还没有填完。", L"检查答案",
                        MB_OK | MB_ICONINFORMATION);
        } else if (IsCorrect(game)) {
            game.message = L"回答正确，恭喜完成本关！";
            MessageBoxW(window, L"全部填写正确！", L"检查答案",
                        MB_OK | MB_ICONINFORMATION);
        } else {
            game.message = L"答案中还有错误，请继续检查";
            MessageBoxW(window, L"答案不正确。", L"检查答案",
                        MB_OK | MB_ICONWARNING);
        }
    } else if (PointIn(SolveButton(), x, y)) {
        for (int row = 0; row < ROW; ++row) {
            for (int column = 0; column < COL; ++column)
                game.board[row][column] = game.solution[row][column];
        }
        game.keypadVisible = false;
        game.message = L"已使用 DPLL 给出本关解析";
    } else if (PointIn(ResetButton(), x, y)) {
        ResetLevel(game);
    } else if (game.keypadVisible && PointIn(KeypadRect(), x, y)) {
        const int column = (x - SIDE_X) / PAD_SIZE;
        const int row = (y - 398) / PAD_SIZE;
        const int digit = row * 3 + column + 1;
        game.board[game.selectedRow][game.selectedColumn] = digit;
        game.keypadVisible = false;
        game.message = L"已填写；再次点击该格可以修改";
    } else {
        game.keypadVisible = false;
    }

    InvalidateRect(window, nullptr, FALSE);
}

LRESULT CALLBACK GameWindowProc(HWND window, UINT message,
                                WPARAM wparam, LPARAM lparam) {
    GameState* game = reinterpret_cast<GameState*>(
        GetWindowLongPtrW(window, GWLP_USERDATA)
    );

    if (message == WM_NCCREATE) {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        game = static_cast<GameState*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(game));
    }

    switch (message) {
        case WM_LBUTTONDOWN:
            if (game != nullptr)
                HandleClick(window, *game, GET_X_LPARAM(lparam),
                            GET_Y_LPARAM(lparam));
            return 0;

        case WM_KEYDOWN:
            if (game != nullptr && game->selectedRow >= 0 &&
                !game->fixed[game->selectedRow][game->selectedColumn] &&
                (wparam == VK_BACK || wparam == VK_DELETE)) {
                game->board[game->selectedRow][game->selectedColumn] = 0;
                game->message = L"已清除当前格";
                InvalidateRect(window, nullptr, FALSE);
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC dc = BeginPaint(window, &paint);
            if (game != nullptr)
                DrawGame(window, dc, *game);
            EndPaint(window, &paint);
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(window, message, wparam, lparam);
}

} // namespace

int Playstar(const std::string& filename) {
    GameState game;
    game.levels = LoadLevels(filename);
    if (!LoadLevel(game, 0))
        throw std::runtime_error("The first star sudoku level has no solution");

    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t className[] = L"DPLLStarSudokuWindow";

    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = GameWindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));

    if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        throw std::runtime_error("Cannot register star sudoku window class");

    HWND window = CreateWindowExW(
        0, className, L"DPLL 星形数独游戏",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 730, 625,
        nullptr, nullptr, instance, &game
    );

    if (window == nullptr)
        throw std::runtime_error("Cannot create star sudoku window");

    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

#else

int Playstar(const std::string&) {
    std::cerr << "The graphical star sudoku game requires Windows.\n";
    return 2;
}

#endif
