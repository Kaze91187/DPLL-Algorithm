#include "Global.h"

#define CORRECT 0
#define WRONG -1
static int T = 0;

static int sudokuVariable(int row, int column, int digit) {
    return (row * 9 + column) * 9 + digit;
}

int Digit(int a[][COL], int i, int j) {//递归填充数独元素
    if (i < ROW && j < COL) {
        int x,y,k;
        int check[COL+1]={CORRECT};//用于排除已经使用过的的数字
        for(x = 0 ; x < i ; x++)
            check[a[x][j]] = WRONG;//列已使用的数字置为WRONG
        for(x = 0 ; x < j ; x++)
            check[a[i][x]] = WRONG;//行使用过的数字置为WRONG
        for(x = i/3*3 ; x <= i; x++) {
            if(x == i)
                for(y = j/3*3 ; y < j; y++)
                    check[a[x][y]] = WRONG;
            else
                for(y = j/3*3 ; y < j/3*3 + 3; y++)
                    check[a[x][y]] = WRONG;
        }

        int flag = 0;
        for(k = 1; k <= COL && flag == 0 ; k++){//从check数组中查找安全的数字
            if(check[k] == CORRECT){
                flag = 1;
                a[i][j] = k;
                if(j == COL-1 && i != ROW-1){
                    if(Digit(a,i+1,0) == CORRECT) return CORRECT;
                    else flag = 0;
                }
                else if(j != COL-1){
                    if(Digit(a,i,j+1) == CORRECT) return CORRECT;
                    else flag = 0;
                }
            }
        }
        if( flag == 0 ) {
            a[i][j] = 0;
            return WRONG;
        }
    }
    return CORRECT;
}

void randomFirstRow(int a0[], int n) {//随机生成第一行
    int i,j;
    srand((unsigned)time(nullptr));
    for( i = 0 ; i < n ; i++){
        a0[i] = rand()%9 + 1;
        j = 0 ;
        while(j < i){
            if(a0[i] == a0[j]){
                a0[i] = rand()%9 + 1;
                j = 0;
            }
            else j++;
        }
    }
}

void createSudoku(int a[][COL]){ //生成数独
    randomFirstRow(a[0],COL);//随机生成第一行
    Digit(a,1,0);//递归生成后i行
}

void createStartinggrid(const int a[][COL], int b[][COL], int numDigits) {//随机生成初盘
    int i,j,k;
    srand((unsigned)time(nullptr));
    for( i = 0; i < ROW ; i ++)
        for( j = 0; j < COL ; j++)
            b[i][j] = a[i][j];

    int c[ROW * COL][2];
    int m,flag = 0;

    for( i = 0; i < numDigits ; i++) {
        j = rand()%9;
        k = rand()%9;

        flag = 0;
        for(m = 0; m < i ; m++)
            if( j == c[m][0] && k == c[m][1])
                flag = 1;

        if(flag == 0){
            b[j][k] = 0;
            c[i][0] = j;
            c[i][1] = k;
        }
        else
            i--;
    }
}

void print(const int a[][COL]){//打印数独数组
    int i,j;
    for( i = 0 ; i < ROW ; i++){
        for( j = 0 ; j < COL ; j++)
            printf("%d ", a[i][j]);
        cout<<endl;
    }
}

string ToCnf(int a[][COL],int holes) {
    ofstream in ("sudoku.cnf");//定义输入文件
    if(!in.is_open())
        cout<<"can't open!\n";
    // 8100 structural clauses plus one unit clause for every given cell.
    in << "p cnf 729 " << 8100 + 81 - holes << '\n';
    //single clause
    for (int x = 0; x < ROW; ++x) {
        for (int y = 0; y < COL; ++y)
            if(a[x][y] != 0)
                in << sudokuVariable(x, y, a[x][y]) << " 0\n";
    }
    //entry
    for (int x = 1; x <= 9; ++x) {
        for (int y = 1; y <= 9; ++y) {
            for (int z = 1; z <= 9; ++z)
                in << sudokuVariable(x - 1, y - 1, z) << " ";
            in<<0;
            in<<endl;
        }
    }
    //row
    for (int y = 1; y <= 9; ++y) {
        for (int z = 1; z <= 9; ++z)
            for (int x = 1; x <= 8; ++x)
                for (int i = x+1; i <= 9; ++i)
                    in << -sudokuVariable(y - 1, x - 1, z) << " "
                       << -sudokuVariable(y - 1, i - 1, z) << " 0\n";
    }
    //column
    for (int x = 1; x <= 9; ++x) {
        for (int z = 1; z <=9 ; ++z)
            for (int y = 1; y <= 8; ++y)
                for (int i = y+1; i <= 9; ++i)
                    in << -sudokuVariable(y - 1, x - 1, z) << " "
                       << -sudokuVariable(i - 1, x - 1, z) << " 0\n";
    }
    //3*3 sub-grids
    for (int z = 1; z <= 9 ; ++z) {
        for (int i = 0; i <=2 ; ++i)
            for (int j = 0; j <=2 ; ++j)
                for (int x = 1; x <= 3 ; ++x)
                    for (int y = 1; y <= 3; ++y)
                        for (int k = y+1; k <= 3; ++k)
                            in << -sudokuVariable(3*i+x-1, 3*j+y-1, z) << " "
                               << -sudokuVariable(3*i+x-1, 3*j+k-1, z) << " 0\n";
    }
    for (int z = 1; z <= 9; z++) {
        for (int i = 0; i <= 2; i++)
            for (int j = 0; j <= 2; j++)
                for (int x = 1; x <= 3; x++)
                    for (int y = 1; y <= 3; y++)
                        for (int k = x + 1; k <= 3; k++)
                            for (int l = 1; l <= 3; l++)
                                in << -sudokuVariable(3*i+x-1, 3*j+y-1, z) << ' '
                                   << -sudokuVariable(3*i+k-1, 3*j+l-1, z) << " 0\n";
    }
    in.close();
    return string("sudoku.cnf");
}

string createSudokuToFile() {
    int sudoku[ROW][COL]={0};
    int starting_grid[ROW][COL]={0};
    int holes = 5;//挖洞个数
    createSudoku(sudoku);//生成数独终盘
    createStartinggrid(sudoku,starting_grid,holes);//生成初盘
    print(starting_grid);//输出初盘
    //转化为cnf文件
    string filename = ToCnf(starting_grid,holes);
    return filename;
}

status SudoDPLL(HeadNode *LIST,conse *result,int VARNUM) {
    //单子句规则
    HeadNode* Pfind = LIST;
    HeadNode* SingleClause = IsSingleClause(Pfind);
    while (SingleClause != nullptr) {
        result[T].num = SingleClause->right->data;
        SingleClause->right->data > 0 ? result[T++].value = TRUE : result[T++].value = FALSE;
        int temp = SingleClause->right->data;
        DeleteHeadNode(SingleClause,LIST);//删除单子句这一行
        DeleteDataNode(temp,LIST);//删除相等或相反数的节点
        if(!LIST) return TRUE;
        else if(IsEmptyClause(LIST)) return FALSE;
        Pfind = LIST;
        SingleClause = IsSingleClause(Pfind);//回到头节点继续进行检测是否有单子句
    }
    //分裂策略
    int Var = LIST->right->data;//选取变元
    HeadNode* replica = Duplication(LIST);//存放LIST的副本replica
    HeadNode *temp1 = ADDSingleClause(LIST,Var);//装载变元成为单子句
    if(SudoDPLL(temp1,result,VARNUM)) return TRUE;
    else {
        HeadNode *temp2 = ADDSingleClause(replica,-Var);
        return SudoDPLL(temp2,result,VARNUM);
    }
}

void SudokuShow(conse *result,int VARNUM) {
    int res[9][9] = {0};
    for (int i = 0; i < VARNUM; ++i) {
        if(result[i].value == TRUE) {
            int x = (int)( abs(result[i].num) / 100 ) - 1;
            int y = (int)( (abs(result[i].num) - (x+1)*100) / 10 ) - 1;
            res[x][y] = abs(result[i].num) - (x+1)*100 - (y+1)*10;
        }
    }
    //输出result数组
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) {
            cout<<res[i][j]<<" ";
        }
        cout<<endl;
    }
}
