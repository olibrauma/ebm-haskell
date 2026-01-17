{-|
Module      : EBM.Types
Description : 型定義 - EBM-on-Mars Haskell実装
Copyright   : (c) 2026
License     : MIT

物理量の型安全なラッパーと、シミュレーションで使用する
主要なデータ構造を定義します。
-}

module EBM.Types where

import qualified Data.Vector.Unboxed as V

-- | 温度 [K]
newtype Temperature = Temperature { unTemperature :: Double }
  deriving (Eq, Ord, Show, Num)

-- | 圧力 [bar]
newtype Pressure = Pressure { unPressure :: Double }
  deriving (Eq, Ord, Show, Num)

-- | 質量 [kg/m^2]
newtype Mass = Mass { unMass :: Double }
  deriving (Eq, Ord, Show, Num)

-- | 角度 [rad]
newtype Angle = Angle { unAngle :: Double }
  deriving (Eq, Ord, Show, Num)

-- | 時間 [s]
newtype Time = Time { unTime :: Double }
  deriving (Eq, Ord, Show, Num)

-- | シミュレーション設定
data SimulationConfig = SimulationConfig
  { resolution :: !Int      -- ^ 緯度分解能（通常181）
  , timeStep :: !Time       -- ^ 時間ステップ [s]
  , dayLength :: !Time      -- ^ 火星の1日の長さ [s]
  , yearLength :: !Time     -- ^ 火星の1年の長さ [s]
  , stepLimit :: !Int       -- ^ 最大ステップ数
  } deriving (Show, Eq)

-- | 惑星パラメータ
data PlanetParams = PlanetParams
  { solarConstant :: !Double    -- ^ 太陽定数 [W/m^2]
  , obliquity :: !Angle         -- ^ 軌道傾斜角 [rad]
  , totalPressure :: !Pressure  -- ^ 全大気圧 [bar]
  , slopeAngleNorth :: !Angle   -- ^ 北向き斜面角度 [rad]
  , slopeAngleSouth :: !Angle   -- ^ 南向き斜面角度 [rad]
  } deriving (Show, Eq)

-- | 気候状態（不変データ構造）
data ClimateState = ClimateState
  { temperatures :: !(V.Vector Double)      -- ^ 温度分布 [K]
  , co2Masses :: !(V.Vector Double)         -- ^ CO2質量分布 [kg/m^2]
  , tempNorthSlope :: !(V.Vector Double)    -- ^ 北向き斜面温度 [K]
  , massNorthSlope :: !(V.Vector Double)    -- ^ 北向き斜面CO2質量 [kg/m^2]
  , tempSouthSlope :: !(V.Vector Double)    -- ^ 南向き斜面温度 [K]
  , massSouthSlope :: !(V.Vector Double)    -- ^ 南向き斜面CO2質量 [kg/m^2]
  , airPressure :: !Pressure                -- ^ 大気圧 [bar]
  , icePressure :: !Pressure                -- ^ 氷圧力 [bar]
  , regolithPressure :: !Pressure           -- ^ レゴリス圧力 [bar]
  , season :: !Time                         -- ^ 季節（時刻） [s]
  , sublimationTemp :: !Temperature         -- ^ 昇華温度 [K]
  , loopCount :: !Int                       -- ^ ループカウント
  , bugFlag :: !Double                      -- ^ エラーフラグ（C版互換）
  } deriving (Show, Eq)

-- | デフォルト設定（火星標準）
defaultConfig :: SimulationConfig
defaultConfig = SimulationConfig
  { resolution = 181
  , timeStep = Time 185.0
  , dayLength = Time (60.0 * (60.0 * 24.0 + 40.0))
  , yearLength = Time 59355072.0  -- calc_Yearsec の結果
  , stepLimit = 10
  }

-- | デフォルト惑星パラメータ
defaultParams :: PlanetParams
defaultParams = PlanetParams
  { solarConstant = 1366.0
  , obliquity = Angle (25.19 * pi / 180.0)
  , totalPressure = Pressure 0.130
  , slopeAngleNorth = Angle (30.0 * pi / 180.0)
  , slopeAngleSouth = Angle (-30.0 * pi / 180.0)
  }

-- | 初期状態の生成
initializeState :: SimulationConfig -> PlanetParams -> ClimateState
initializeState config params =
  let n = resolution config
      initTemp = 250.0
  in ClimateState
    { temperatures = V.replicate n initTemp
    , co2Masses = V.replicate n 0.0
    , tempNorthSlope = V.replicate n initTemp
    , massNorthSlope = V.replicate n 0.0
    , tempSouthSlope = V.replicate n initTemp
    , massSouthSlope = V.replicate n 0.0
    , airPressure = totalPressure params
    , icePressure = Pressure 0.0
    , regolithPressure = Pressure 0.0
    , season = Time 0.0
    , sublimationTemp = Temperature 0.0
    , loopCount = 0
    , bugFlag = 0.0
    }
