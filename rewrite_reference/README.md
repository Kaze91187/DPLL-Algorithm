# 复刻顺序

这套代码不修改原项目，删除了旧版 `SudoDPLL` 和冗余结果结构，只保留一套 DPLL。

当前版本包含两项主要优化：

1. 使用赋值轨迹和递归检查点回溯，只撤销当前分支产生的赋值，不再在每层复制整个赋值数组。
2. 使用短子句加权的分支策略，分别统计正、负文字得分，选择总得分最高的变量，并优先探索得分更高的真假方向。权重通过左移计算。

`rewrite_dpll_baseline.exe` 是优化前基线，`rewrite_dpll_optimized.exe` 是第一版优化，`rewrite_dpll_storage.exe` 是第二版连续存储优化，`rewrite_dpll_incremental.exe` 是第三版增量传播优化，`rewrite_dpll.exe` 默认指向第三版。

第二版保留用于解析与人工验证的二维链表，在进入 DPLL 时转换为连续存储：

- `literals[]` 连续保存全部文字；
- `clauseOffsets[]` 保存每个子句在文字数组中的起止位置；
- 正负文字评分数组只在 DPLL 入口申请一次，在递归中复用。

第一版源码是 `DPLLSolver.cpp`，第二版源码是 `DPLLSolverStorage.cpp`，编译时二选一，不能同时加入编译命令。

第三版源码是 `DPLLSolverIncremental.cpp`，在连续存储上增加：

- 正负文字到相关子句的压缩出现表；
- 每个子句的未赋值文字数和满足文字数；
- 单子句传播队列；
- 赋值和撤销时仅增量更新受影响子句。

第三版仍是DPLL，只改变公式状态维护和单子句传播方法。

建议按以下顺序手写并测试：

1. `Global.h`：定义文字结点、子句结点和函数接口。
2. `CnfParser.cpp`：读取 DIMACS 文件，建立二维链表。
3. `DPLLSolver.cpp`：判断子句状态、单子句传播、选取变量、递归分支和回溯。
4. `main.cpp`：读取命令行、计时、输出 `.res`。
5. `StarSudoku.cpp`：将格、行、列、宫、星形区域约束加入公式，再调用同一个 DPLL。
6. `StarSudokuGame.cpp`：读取题库并建立 Windows 图形游戏界面。

## 编译

```powershell
g++ -std=c++11 -O2 -Wall -Wextra -pedantic `
  main.cpp CnfParser.cpp DPLLSolverIncremental.cpp StarSudoku.cpp StarSudokuGame.cpp `
  -lgdi32 -luser32 -o rewrite_dpll.exe
```

## 运行

```powershell
.\rewrite_dpll.exe ..\problems\sat-20.cnf --verify
.\rewrite_dpll.exe --star <星形数独文件>
.\rewrite_dpll.exe --game <星形数独文件>
```

游戏模式会把文件中的每个合法 81 字符格局作为一关。点击非提示格后，
右侧显示 1～9 的 3×3 小键盘；可以检查完整答案、随时直接解析、重置本关，
或切换上一关和下一关。选中玩家填写的格子后，也可以按退格键或 Delete 清空。

项目附带 `Asterisk-sudoku-levels.txt`，其中包含 20 个可解关卡。直接双击
`rewrite_dpll.exe`，或在终端中不带参数运行它，会自动读取这个内置题库并进入游戏。

详细测试数据见 `BENCHMARK.md`。
