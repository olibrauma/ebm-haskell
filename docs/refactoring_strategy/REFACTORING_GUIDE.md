# AIエージェント向けリファクタリング・ガイドブック

本ドキュメントは、EBM-on-Marsプロジェクトのリファクタリングを安全かつ確実に実行するための詳細な手順書です。

## 1. 検証用「ゴールデンマスター」の作成方法

リファクタリングに着手する前に、現状の出力を保存する必要があります。

### 1.1 キャプチャすべき出力
以下の変数を、シミュレーション開始から 100 ステップ目までの各ステップで記録するテスト用ラッパーを作成してください。
- `double T[181]`, `double M[181]`
- `double T_posi[181]`, `double M_posi[181]`, `double T_nega[181]`, `double M_nega[181]`
- `double P_air`, `double P_ice`, `double P_rego`
- `double season`

### 1.2 比較スクリプト
Python または C で、2つの出力ファイルの全浮動小数点数が、相対誤差 $10^{-12}$ 以内であることを検証するスクリプトを用意してください。

## 2. 推奨されるデータ構造（C言語）

以下の構造体への移行を推奨します。

```c
typedef struct {
    double Q;           // 太陽定数
    double obl;         // 惑星傾斜角
    double P_total;     // 総大気量
    double alpha;       // 斜面角度
    // ... その他物理定数
} PlanetParams;

typedef struct {
    double T[181];
    double M[181];
    double T_posi[181];
    double M_posi[181];
    double T_nega[181];
    double M_nega[181];
    double P_air;
    double P_ice;
    double P_rego;
    double season;
} ClimateState;
```

## 3. 段階的リファクタリング手順

### Step 1: 関数シグネチャの統一
全ての物理計算関数を、以下の形式に統一します。
`void calc_xxx(const PlanetParams *params, ClimateState *state, double dt);`

### Step 2: 依存性の排除
`main` 関数内のローカル変数への直接アクセスを止め、全て構造体経由でデータを受け渡すようにします。
**注意**: `calc_dif` 等で隣接する格子点 (`T[n+1]`) を参照する場合の境界条件（0, 180）は慎重に維持してください。

### Step 3: モジュール分割
ヘッダーファイル (`ebm.h`) を作成し、型定義とプロトタイプ宣言をまとめます。その後、物理カテゴリごとに `.c` ファイルを切り出します。

## 4. 重い計算の回避と広範囲テストの両立

### 4.1 ショートラン・テスト
リファクタリングの各変更ごとに、以下の3つのケースで 50 ステップのみ実行し、誤差を確認します。
1. **標準火星**: `P_total = 0.007`, `obl = 25.19`
2. **高圧・高傾斜**: `P_total = 1.0`, `obl = 45.0`
3. **無大気極寒**: `P_total = 0.0001`, `obl = 5.0`

### 4.2 特異点テスト
惑星科学的に特殊な条件で計算が破綻しないか確認します。
- `P_total` が極めて小さい場合（相変化の挙動）。
- `obl = 0` または `obl = 90`（日射量の分布が極端になる）。

## 5. 完了定義 (Definition of Done)
1. 全てのコードが `ebm.h`, `insolation.c`, `atmosphere.c`, `phase_change.c`, `main.c` に分割されている。
2. グローバル変数が存在せず、状態が構造体で明示的に受け渡されている。
3. [1.2] の比較スクリプトにより、元実装との差異が誤差範囲内であることが証明されている。
4. `Makefile` でクリーンにビルドでき、`-Wall -Wextra` で警告が出ない。
