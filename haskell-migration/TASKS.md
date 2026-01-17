# Haskell移植タスクリスト

## Phase 1: 基盤構築
- [x] Stackプロジェクト初期化 (`stack new`) <!-- id: 0 -->
- [x] `package.yaml` 設定（依存パッケージ） <!-- id: 1 -->
- [x] ディレクトリ構造作成 (`src/EBM/`) <!-- id: 2 -->
- [x] `Types.hs` 作成（基本型定義） <!-- id: 3 -->
- [x] `Config.hs` 作成（設定・パラメータ） <!-- id: 4 -->
- [x] ゴールデンマスターデータ準備 <!-- id: 5 -->
- [x] 検証フレームワーク (`Verification.hs`) <!-- id: 6 -->

## Phase 2: コア物理モジュール移植
- [x] `Insolation.hs` 実装 <!-- id: 7 -->
  - [x] `calcYearLength` 関数
  - [x] `calcDelta` 関数（軌道パラメータ）
  - [x] `calcInsolation` 関数（グローバル日射）
  - [x] `calcSlopeInsolation` 関数（斜面日射）
  - [x] 単体テスト・検証
- [x] `Atmosphere.hs` 実装 <!-- id: 8 -->
  - [x] `calcDiffusion` 関数
  - [x] `calcRadiation` 関数
  - [x] `calcSlopeDiffusion` 関数
  - [x] 単体テスト・検証
- [x] `PhaseChange.hs` 実装 <!-- id: 9 -->
  - [x] `sublimationParams` 関数
  - [x] `calcTemperatureMass` 関数
  - [x] `calcSlopeTemperatureMass` 関数
  - [x] `calcIcePressure` 関数
  - [x] `calcRegolithPressure` 関数
  - [x] 単体テスト・検証

## Phase 3: シミュレーション制御
- [ ] `Simulation.hs` 実装 <!-- id: 10 -->
  - [ ] `equilibrium` 関数（平衡計算）
  - [ ] `timeStep` 関数（1ステップ時間発展）
  - [ ] 統合テスト
- [ ] `IO.hs` 実装 <!-- id: 11 -->
  - [ ] `dumpState` 関数
  - [ ] `parseArgs` 関数
- [ ] `Main.hs` 実装 <!-- id: 12 -->
  - [ ] メインループ
  - [ ] コマンドライン引数処理

## Phase 4: 検証
- [ ] 標準ケース検証 (P=0.007, Obl=25.19, Alpha=30) <!-- id: 13 -->
- [ ] 極端ケース検証 (P=5.0, Obl=90, Alpha=45) <!-- id: 14 -->
- [ ] 数値精度確認 (1e-10) <!-- id: 15 -->
- [ ] 全パラメータグリッド検証（オプション） <!-- id: 16 -->

## Phase 5: 最適化・拡張
- [ ] 性能プロファイリング <!-- id: 17 -->
- [ ] 並列化検討 <!-- id: 18 -->
- [ ] ドキュメント整備 <!-- id: 19 -->
- [ ] README作成 <!-- id: 20 -->
