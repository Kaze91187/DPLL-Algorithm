# 复刻顺序

这套代码不修改原项目，删除了旧版 `SudoDPLL` 和冗余结果结构，只保留一套 DPLL。

当前版本包含两项主要优化：

1. 使用赋值轨迹和递归检查点回溯，只撤销当前分支产生的赋值，不再在每层复制整个赋值数组。
2. 使用短子句加权的分支策略，分别统计正、负文字得分，选择总得分最高的变量，并优先探索得分更高的真假方向。权重通过左移计算。

`rewrite_dpll_baseline.exe` 是优化前基线，`rewrite_dpll_optimized.exe` 是优化版，`rewrite_dpll.exe` 默认指向优化版。

建议按以下顺序手写并测试：

1. `Global.h`：定义文字结点、子句结点和函数接口。
2. `CnfParser.cpp`：读取 DIMACS 文件，建立二维链表。
3. `DPLLSolver.cpp`：判断子句状态、单子句传播、选取变量、递归分支和回溯。
4. `main.cpp`：读取命令行、计时、输出 `.res`。
5. `StarSudoku.cpp`：将格、行、列、宫、星形区域约束加入公式，再调用同一个 DPLL。

## 编译

```powershell
g++ -std=c++11 -O2 -Wall -Wextra -pedantic `
  main.cpp CnfParser.cpp DPLLSolver.cpp StarSudoku.cpp `
  -o rewrite_dpll_optimized.exe
```

## 运行

```powershell
.\rewrite_dpll_optimized.exe ..\problems\sat-20.cnf --verify
.\rewrite_dpll_optimized.exe --star <星形数独文件>
```

详细测试数据见 `BENCHMARK.md`。
