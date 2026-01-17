# Haskell移植実装計画 - EBM-on-Mars

## 1. 目的
検証済みのC言語実装（モジュール化済み）をHaskellに移植し、数値的等価性を保ちながら、Haskellの型安全性と関数型プログラミングの利点を活用する。

## 2. 移植の基本方針

### 2.1 段階的移植アプローチ
C言語リファクタリングで成功した方法論を踏襲：
- **ゴールデンマスター検証**: C版の出力を正解データとして使用
- **モジュール単位の移植**: 物理プロセスごとに独立して移植・検証
- **高精度検証**: 1e-10精度での数値一致を目標

### 2.2 Haskell設計原則
- **純粋関数**: 物理計算は全て純粋関数として実装
- **型安全性**: 物理量に適切な型を定義（newtype wrappers）
- **不変性**: 状態更新は新しい値を返す形式
- **モジュール分割**: C版と同じ物理プロセス単位

## 3. 型設計

### 3.1 基本型定義
```haskell
-- 物理量の型安全なラッパー
newtype Temperature = Temperature Double  -- [K]
newtype Pressure = Pressure Double        -- [bar]
newtype Mass = Mass Double                -- [kg/m^2]
newtype Angle = Angle Double              -- [rad]
newtype Time = Time Double                -- [s]

-- 設定パラメータ
data SimulationConfig = SimulationConfig
  { resolution :: Int
  , timeStep :: Time
  , dayLength :: Time
  , yearLength :: Time
  , stepLimit :: Int
  }

data PlanetParams = PlanetParams
  { solarConstant :: Double
  , obliquity :: Angle
  , totalPressure :: Pressure
  , slopeAngleNorth :: Angle
  , slopeAngleSouth :: Angle
  }

-- 気候状態（不変データ構造）
data ClimateState = ClimateState
  { temperatures :: Vector Temperature
  , co2Masses :: Vector Mass
  , tempNorthSlope :: Vector Temperature
  , massNorthSlope :: Vector Mass
  , tempSouthSlope :: Vector Temperature
  , massSouthSlope :: Vector Mass
  , airPressure :: Pressure
  , icePressure :: Pressure
  , regolithPressure :: Pressure
  , season :: Time
  , sublimationTemp :: Temperature
  , loopCount :: Int
  }
```

### 3.2 モジュール構造
```
src/
├── EBM/
│   ├── Types.hs           -- 型定義
│   ├── Config.hs          -- 設定・パラメータ
│   ├── Insolation.hs      -- 日射量計算
│   ├── Atmosphere.hs      -- 大気プロセス
│   ├── PhaseChange.hs     -- 相変化
│   ├── Simulation.hs      -- シミュレーション制御
│   └── IO.hs              -- 入出力
├── Main.hs                -- エントリーポイント
└── Verification.hs        -- 検証ユーティリティ
```

## 4. 実施フェーズ

### Phase 1: 基盤構築
- [x] プロジェクト構造作成
- [ ] 型定義 (`Types.hs`, `Config.hs`)
- [ ] ゴールデンマスターデータ準備（C版出力を使用）
- [ ] 検証フレームワーク構築

### Phase 2: コア物理モジュール移植
- [ ] `Insolation.hs`: 日射量計算
  - `calcInsolation`: グローバル日射
  - `calcDelta`: 軌道パラメータ
  - `calcSlopeInsolation`: 斜面日射
- [ ] `Atmosphere.hs`: 大気プロセス
  - `calcDiffusion`: 拡散
  - `calcRadiation`: 放射
- [ ] `PhaseChange.hs`: 相変化
  - `calcTemperatureMass`: 温度・質量更新
  - `calcIce`: 氷圧力
  - `calcRegolith`: レゴリス圧力

### Phase 3: シミュレーション制御
- [ ] `Simulation.hs`: 平衡計算・時間発展
  - `equilibrium`: 平衡状態計算
  - `timeStep`: 1ステップ時間発展
- [ ] `IO.hs`: 入出力
  - `dumpState`: 状態出力
  - `parseArgs`: コマンドライン引数

### Phase 4: 検証
- [ ] 標準ケース検証 (P=0.007, Obl=25.19, Alpha=30)
- [ ] 極端ケース検証 (P=5.0, Obl=90, Alpha=45)
- [ ] 数値精度確認 (1e-10目標)

### Phase 5: 最適化・拡張
- [ ] 並列化 (`parallel`, `repa`パッケージ検討)
- [ ] 性能プロファイリング
- [ ] ドキュメント整備

## 5. 検証戦略

### 5.1 ゴールデンマスター検証
```haskell
-- 検証関数の型
verifyAgainstC :: FilePath -> ClimateState -> IO Bool
```
- C版の `dump_*.dat` ファイルと比較
- 許容誤差: `1e-10` (相対・絶対)
- 全181緯度点 × 6変数を検証

### 5.2 段階的検証
各モジュール移植後に単体テスト：
```haskell
-- 例: 日射量計算の検証
testInsolation :: Test
testInsolation = TestCase $ do
  let result = calcInsolation config params state
  golden <- readGoldenData "insolation_golden.dat"
  assertClose 1e-10 result golden
```

## 6. 技術的課題と対策

### 6.1 数値精度
- **課題**: Haskellの浮動小数点演算の精度
- **対策**: 
  - `Double` 使用（C言語と同じ）
  - 演算順序をC版と完全一致させる
  - 必要に応じて `Numeric.IEEE` 使用

### 6.2 配列処理
- **課題**: C言語の配列操作との等価性
- **対策**:
  - `Data.Vector.Unboxed` 使用（メモリ効率）
  - インデックスアクセスパターンを保持
  - 境界チェックに注意

### 6.3 状態管理
- **課題**: C言語の破壊的更新 vs Haskellの不変性
- **対策**:
  - `State` モナド使用（必要に応じて）
  - レンズライブラリ検討（複雑な更新用）
  - 基本は純粋関数で新しい状態を返す

## 7. 依存パッケージ

```yaml
dependencies:
  - base >= 4.7 && < 5
  - vector >= 0.12
  - mtl >= 2.2          # State monad
  - optparse-applicative # コマンドライン引数
  - bytestring
  - text
  - scientific          # 高精度数値
```

## 8. ビルド・実行環境

```yaml
# stack.yaml
resolver: lts-21.25  # GHC 9.4.8
packages:
  - .
```

```yaml
# package.yaml
name: ebm-mars-haskell
version: 0.1.0.0
ghc-options:
  - -Wall
  - -O2
  - -fno-warn-unused-do-bind
```

## 9. 次のステップ

1. Stackプロジェクト初期化
2. `Types.hs` と `Config.hs` 作成
3. 検証フレームワーク構築
4. `Insolation.hs` から段階的移植開始
5. 各モジュール完成後にゴールデンマスター検証

## 10. 成功基準

- [ ] 全モジュールが型チェックを通過
- [ ] 標準ケースで1e-10精度一致
- [ ] 極端ケースで1e-10精度一致
- [ ] C版と同等以上の実行速度（最適化後）
- [ ] 完全なドキュメント整備
