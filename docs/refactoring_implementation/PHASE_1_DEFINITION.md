# 第一段階（Phase 1）の改修内容定義

Phase 1 では、`test.c` の単一ファイル構造を維持したまま、データ保持の形態を構造体へと移行します。

## 1. 導入する構造体

以下の3つの構造体を定義します。

### 1.1 `SimulationConfig` (不変の設定)
シミュレーションの実行条件を保持します。
- `int reso`: 緯度方向の解像度 (181)
- `double dt`: 時間ステップ (185.0)
- `double day_sec`: 1日の秒数
- `double Year_sec`: 1年の秒数

### 1.2 `PlanetParams` (物理的なパラメータ)
惑星の物理的属性を保持します。
- `double obl`: 自転軸傾斜角
- `double P_total`: 総大気圧
- `double alpha_posi/nega`: 斜面の角度
- その他、`calc_delta` 等で使用される軌道要素定数

### 1.3 `ClimateState` (時間とともに変化する状態)
シミュレーションの内部状態を保持します。
- `double T[181]`, `double M[181]`
- `double T_posi[181]`, `double M_posi[181]`
- `double T_nega[181]`, `double M_nega[181]`
- `double P_air`, `double P_ice`, `double P_rego`
- `double season`: 現在の時刻（秒）

## 2. 関数のシグネチャ変更

全ての関数を、構造体ポインタを受け取る形式に統一します。

**例 (現状):**
```c
void calc_ins(double season, double obl, double ins[]);
```

**例 (Phase 1 修正後):**
```c
void calc_ins(const SimulationConfig *config, const PlanetParams *params, ClimateState *state, double ins[]);
```

## 3. main 関数の役割の変更

`main` 関数は以下の役割に専念するようにリファクタリングします。
1.  各構造体（`config`, `params`, `state`）の宣言と初期化。
2.  `heikou` 関数の呼び出し（構造体ポインタを渡す）。
3.  メインループの実行とファイル出力。

## 4. 安全性の保証
この段階での改修は、**ロジックには一切手を触れず、変数の参照先を構造体メンバーに変えるだけ**の作業とします。これにより、次のステップで行う「Golden Master（状態ダンプの比較）」による検証が極めて容易になります。
