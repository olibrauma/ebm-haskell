# ebm-mars (Haskell 移植版)

火星のエネルギーバランスモデル（EBM）のモダンなHaskell実装です。オリジナルのC実装に対し、高精度な交差言語テストによって正確性が検証されています。

## 特徴

- **型安全な物理量**: `Temperature`, `Pressure`, `Mass` などの新しい型（newtype）を定義し、単位の混同を防ぎます。
- **高速な数値積分**: `Data.Vector.Unboxed` を活用し、格子点ごとの緯度計算を効率化しています。
- **高精度な一貫性検証**: 80ケースのゴールデンマスター（C製正解データ）に対し、完全に自動化されたスイープテストを備えています。

## 準備

- [Stack](https://docs.haskellstack.org/en/stable/README/) (Haskell ビルドツール)
- [Python 3](https://www.python.org/) (検証スクリプト実行用)

## ビルドと実行

### プロジェクトのビルド
```bash
stack build
```

### シミュレーションの実行
コマンドライン引数を指定して実行できます。
```bash
stack exec -- ebm-mars-exe [気圧] [赤道傾斜角] [斜面角度] [出力ディレクトリ]
```
例（現在の火星）:
```bash
stack exec -- ebm-mars-exe 0.007 25.19 30.0 ./output
```

### 正確性の検証
高精度なゴールデンマスターとの自動比較を実行します。
```bash
python3 verify.py
```
*(注: 実行には golden-master ディレクトリが必要です)*

## リポジトリのコンテキスト
本コードは、2025年〜2026年にかけて実施された系統的なリファクタリングと移行フェーズの成果物です。詳細な検証手法については、[プロジェクトルートの README](../../README.md) を参照してください。
