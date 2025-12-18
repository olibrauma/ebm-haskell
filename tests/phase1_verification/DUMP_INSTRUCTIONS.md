# オリジナルプログラムへのダンプコード埋め込み手順

リファクタリング前の `test.c` から正解データを抽出するため、以下の場所にダンプ処理を挿入します。

## 1. ダンプ関数の追加
`test.c` の末尾、または `main` の直前に以下の関数を追加します。

```c
void dump_state(const char* filename, int loop, double season, double P_air, double P_ice, double P_rego, double T_sub, 
                double T[], double M[], double T_posi[], double M_posi[], double T_nega[], double M_nega[]) {
    FILE *f = fopen(filename, "w");
    if (!f) return;
    fprintf(f, "loop:%d,season:%g,P_air:%g,P_ice:%g,P_rego:%g,T_sub:%g\n", loop, season, P_air, P_ice, P_rego, T_sub);
    for(int i=0; i<181; i++) {
        fprintf(f, "%d,%g,%g,%g,%g,%g,%g\n", i, T[i], M[i], T_posi[i], M_posi[i], T_nega[i], M_nega[i]);
    }
    fclose(f);
}
```

## 2. main関数内での呼び出し (挿入箇所)

### A. パラメータの上書き (テスト実行用)
`main` 関数の最初（初期化後、`heikou` 呼び出し前）に、外部からパラメータを与えられるようにします。

```c
// 挿入ポイント: line 77 付近
// if (argc > 3) { P_total = atof(argv[1]); obl = atof(argv[2])*M_PI/180.0; alpha_posi = atof(argv[3])*M_PI/180.0; alpha_nega = -alpha_posi; }
```

### B. 平衡状態後のダンプ
`heikou` 終了直後に初期状態を記録します。

```c
// 挿入ポイント: line 80 付近
dump_state("dump_000.dat", loop, season, P_air, P_ice, P_rego, T_sub, T, M, T_posi, M_posi, T_nega, M_nega);
```

### C. ステップごとのダンプ
メインループ内の最後に呼び出しを追加します。ただし、重い計算を避けるため **100ステップで終了** するように条件を追加します。

```c
// 挿入ポイント: line 149 (whileの直前)
static int step_count = 0;
char fname[64];
sprintf(fname, "dump_%03d.dat", ++step_count);
dump_state(fname, loop, season, P_air, P_ice, P_rego, T_sub, T, M, T_posi, M_posi, T_nega, M_nega);
if (step_count >= 100) break; // 100ステップで強制終了
```

## 3. 注意点
- `main` 関数の引数を `int main(int argc, char *argv[])` に変更する必要があります。
- 構造体導入後のプログラムでも、全く同じ場所でこのダンプ関数（引数は構造体経由になりますが）を呼び出すように実装します。
