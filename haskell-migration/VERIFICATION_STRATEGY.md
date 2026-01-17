# 検証戦略詳細 - Haskell移植

## 1. ゴールデンマスター検証

### 1.1 検証データの準備

C言語版（検証済み）の出力を正解データとして使用：

```bash
# 標準ケース
./test_modular 0.007 25.19 30.0 golden_master/standard/

# 極端ケース
./test_modular 5.0 90.0 45.0 golden_master/extreme/
```

### 1.2 検証関数の実装

```haskell
-- Verification.hs
module Verification where

import qualified Data.Vector.Unboxed as V
import Data.Scientific (Scientific, toRealFloat)

data VerificationResult = VerificationResult
  { passed :: Bool
  , maxRelativeError :: Double
  , maxAbsoluteError :: Double
  , failedPoints :: [(Int, String, Double, Double)]
  } deriving (Show)

-- メイン検証関数
verifyDumpFile :: FilePath -> FilePath -> IO VerificationResult
verifyDumpFile goldenPath testPath = do
  golden <- parseDumpFile goldenPath
  test <- parseDumpFile testPath
  return $ compareStates golden test

-- 状態比較（1e-10精度）
compareStates :: ClimateState -> ClimateState -> VerificationResult
compareStates golden test =
  let tolerance = 1e-10
      checks = 
        [ checkVector "T" (temperatures golden) (temperatures test)
        , checkVector "M" (co2Masses golden) (co2Masses test)
        , checkVector "T_posi" (tempNorthSlope golden) (tempNorthSlope test)
        , checkVector "M_posi" (massNorthSlope golden) (massNorthSlope test)
        , checkVector "T_nega" (tempSouthSlope golden) (tempSouthSlope test)
        , checkVector "M_nega" (massSouthSlope golden) (massSouthSlope test)
        ]
      allPassed = all passed checks
      maxRelErr = maximum $ map maxRelativeError checks
      maxAbsErr = maximum $ map maxAbsoluteError checks
  in VerificationResult allPassed maxRelErr maxAbsErr []

-- ベクトル比較
checkVector :: String -> V.Vector Double -> V.Vector Double -> VerificationResult
checkVector name golden test =
  let n = V.length golden
      tolerance = 1e-10
      diffs = V.zipWith (\g t -> (abs (g - t), abs ((g - t) / g))) golden test
      failures = V.ifilter (\i (absDiff, relDiff) -> 
                    absDiff > tolerance && relDiff > tolerance) diffs
  in if V.null failures
     then VerificationResult True 0.0 0.0 []
     else let (maxAbs, maxRel) = V.maximum diffs
          in VerificationResult False maxRel maxAbs []
```

## 2. 段階的検証戦略

### Phase 2.1: Insolation モジュール検証

```haskell
-- test/InsolationSpec.hs
spec :: Spec
spec = describe "Insolation calculations" $ do
  
  it "calcYearLength matches C implementation" $ do
    let yearSec = calcYearLength
    yearSec `shouldSatisfy` (\y -> abs (y - 59355072.0) < 1e-6)
  
  it "calcDelta produces correct orbital parameters" $ do
    let config = defaultConfig
        params = defaultParams
        (delta, rAU) = calcDelta config params (Time 0.0)
    -- C版の出力と比較
    delta `shouldBeClose` (-0.4014257) $ 1e-10
    rAU `shouldBeClose` 1.6660261 $ 1e-10
  
  it "calcInsolation matches golden master" $ do
    golden <- readGoldenInsolation "test/data/insolation_golden.dat"
    let result = calcInsolation config params state
    result `shouldMatchVector` golden $ 1e-10
```

### Phase 2.2: Atmosphere モジュール検証

```haskell
spec :: Spec
spec = describe "Atmosphere calculations" $ do
  
  it "calcDiffusion matches C implementation" $ do
    golden <- readGoldenDiffusion "test/data/diffusion_golden.dat"
    let result = calcDiffusion config params state
    result `shouldMatchVector` golden $ 1e-10
  
  it "calcRadiation matches C implementation" $ do
    golden <- readGoldenRadiation "test/data/radiation_golden.dat"
    let result = calcRadiation config params state
    result `shouldMatchVector` golden $ 1e-10
```

### Phase 2.3: PhaseChange モジュール検証

```haskell
spec :: Spec
spec = describe "Phase change calculations" $ do
  
  it "calcIce produces correct ice pressure" $ do
    let result = calcIce config params state
    result `shouldBeClose` goldenPice $ 1e-10
  
  it "calcRegolith produces correct regolith pressure" $ do
    let (pAir, pRego) = calcRegolith config params state
    pAir `shouldBeClose` goldenPair $ 1e-10
    pRego `shouldBeClose` goldenPrego $ 1e-10
```

## 3. 統合検証

### 3.1 完全シミュレーション検証

```haskell
-- test/IntegrationSpec.hs
spec :: Spec
spec = describe "Full simulation" $ do
  
  it "standard case matches C output exactly" $ do
    let config = defaultConfig
        params = standardParams  -- P=0.007, Obl=25.19, Alpha=30
        initialState = initializeState config params
    
    -- 平衡計算
    let eqState = equilibrium config params initialState
    
    -- 10ステップ時間発展
    let finalStates = take 11 $ iterate (timeStep config params) eqState
    
    -- 各ステップのダンプファイルと比較
    forM_ (zip [0..10] finalStates) $ \(i, state) -> do
      let goldenPath = "golden_master/standard/dump_" ++ printf "%03d" i ++ ".dat"
      result <- verifyDumpFile goldenPath state
      result `shouldBe` VerificationResult True 0.0 0.0 []
  
  it "extreme case matches C output" $ do
    let params = extremeParams  -- P=5.0, Obl=90, Alpha=45
    -- 同様の検証
```

## 4. 数値精度保証

### 4.1 浮動小数点演算の注意点

```haskell
-- 演算順序をC版と完全一致させる
-- 悪い例:
badCalc x y z = (x + y) + z

-- 良い例（C版と同じ順序）:
goodCalc x y z = x + (y + z)

-- 必要に応じて明示的な型注釈
explicitCalc :: Double -> Double -> Double
explicitCalc x y = x * y
```

### 4.2 特殊ケースの処理

```haskell
-- C版の条件分岐を正確に再現
calcRadiation :: PlanetParams -> ClimateState -> Vector Double
calcRadiation params state =
  V.generate (resolution config) $ \n ->
    let temp = temperatures state V.! n
        -- C版: if (state->T[n] > 230.1)
        coeffs = if unTemperature temp > 230.1
                 then highTempCoeffs
                 else lowTempCoeffs
    in computeRadiation coeffs temp
```

## 5. 継続的検証

### 5.1 自動テストスイート

```yaml
# .github/workflows/verify.yml
name: Verification
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - uses: haskell/actions/setup@v1
      - run: stack test --coverage
      - run: stack exec verify-against-c
```

### 5.2 性能ベンチマーク

```haskell
-- bench/Benchmark.hs
import Criterion.Main

main :: IO ()
main = defaultMain
  [ bgroup "simulation"
    [ bench "standard case" $ nf (runSimulation standardParams) 10
    , bench "extreme case" $ nf (runSimulation extremeParams) 10
    ]
  ]
```

## 6. 検証チェックリスト

- [ ] 全モジュールの単体テストが通過
- [ ] 標準ケースで1e-10精度一致（11ファイル全て）
- [ ] 極端ケースで1e-10精度一致（11ファイル全て）
- [ ] 既知の不一致（dump_000.dat の T_sub ヘッダ）を文書化
- [ ] 性能がC版の80%以上（最適化後）
- [ ] メモリ使用量が妥当な範囲
