{-|
Module      : EBM.Types
Description : Core type definitions for EBM simulation
Copyright   : (c) 2026
License     : MIT

This module defines type-safe wrappers for physical quantities
and the main data structures used in the simulation.
-}

{-# LANGUAGE DeriveGeneric #-}
{-# LANGUAGE GeneralizedNewtypeDeriving #-}

module EBM.Types
  ( -- * Physical quantity types
    Temperature(..)
  , Pressure(..)
  , Mass(..)
  , Angle(..)
  , Time(..)
    -- * Configuration types
  , SimulationConfig(..)
  , PlanetParams(..)
  , ClimateState(..)
    -- * Default values
  , defaultConfig
  , defaultParams
  , initializeState
  ) where

import qualified Data.Vector.Unboxed as V
import GHC.Generics (Generic)

-- | Temperature [K]
newtype Temperature = Temperature { unTemperature :: Double }
  deriving (Eq, Ord, Show, Num, Fractional, Floating, Real, RealFrac, RealFloat, Generic)

-- | Pressure [bar]
newtype Pressure = Pressure { unPressure :: Double }
  deriving (Eq, Ord, Show, Num, Fractional, Floating, Real, RealFrac, RealFloat, Generic)

-- | Mass [kg/m^2]
newtype Mass = Mass { unMass :: Double }
  deriving (Eq, Ord, Show, Num, Fractional, Floating, Real, RealFrac, RealFloat, Generic)

-- | Angle [rad]
newtype Angle = Angle { unAngle :: Double }
  deriving (Eq, Ord, Show, Num, Fractional, Floating, Real, RealFrac, RealFloat, Generic)

-- | Time [s]
newtype Time = Time { unTime :: Double }
  deriving (Eq, Ord, Show, Num, Fractional, Floating, Real, RealFrac, RealFloat, Generic)

-- | Simulation configuration parameters
data SimulationConfig = SimulationConfig
  { resolution :: !Int      -- ^ Latitude resolution (typically 181)
  , timeStep :: !Time       -- ^ Time step [s]
  , dayLength :: !Time      -- ^ Martian day length [s]
  , yearLength :: !Time     -- ^ Martian year length [s]
  , stepLimit :: !Int       -- ^ Maximum number of steps
  } deriving (Show, Eq, Generic)

-- | Planet physical parameters
data PlanetParams = PlanetParams
  { solarConstant :: !Double    -- ^ Solar constant [W/m^2]
  , obliquity :: !Angle         -- ^ Orbital obliquity [rad]
  , totalPressure :: !Pressure  -- ^ Total atmospheric pressure [bar]
  , slopeAngleNorth :: !Angle   -- ^ North-facing slope angle [rad]
  , slopeAngleSouth :: !Angle   -- ^ South-facing slope angle [rad]
  } deriving (Show, Eq, Generic)

-- | Climate state (immutable data structure)
data ClimateState = ClimateState
  { temperatures :: !(V.Vector Double)      -- ^ Temperature distribution [K]
  , co2Masses :: !(V.Vector Double)         -- ^ CO2 mass distribution [kg/m^2]
  , tempNorthSlope :: !(V.Vector Double)    -- ^ North slope temperature [K]
  , massNorthSlope :: !(V.Vector Double)    -- ^ North slope CO2 mass [kg/m^2]
  , tempSouthSlope :: !(V.Vector Double)    -- ^ South slope temperature [K]
  , massSouthSlope :: !(V.Vector Double)    -- ^ South slope CO2 mass [kg/m^2]
  , airPressure :: !Pressure                -- ^ Atmospheric pressure [bar]
  , icePressure :: !Pressure                -- ^ Ice pressure [bar]
  , regolithPressure :: !Pressure           -- ^ Regolith pressure [bar]
  , season :: !Time                         -- ^ Season (time) [s]
  , sublimationTemp :: !Temperature         -- ^ Sublimation temperature [K]
  , loopCount :: !Int                       -- ^ Loop counter
  , bugFlag :: !Double                      -- ^ Error flag (C compatibility)
  } deriving (Show, Eq, Generic)

-- | Default configuration (Mars standard)
defaultConfig :: SimulationConfig
defaultConfig = SimulationConfig
  { resolution = 181
  , timeStep = Time 185.0
  , dayLength = Time (60.0 * (60.0 * 24.0 + 40.0))
  , yearLength = Time 59355072.0  -- Result of calcYearLength
  , stepLimit = 10
  }

-- | Default planet parameters
defaultParams :: PlanetParams
defaultParams = PlanetParams
  { solarConstant = 1366.0
  , obliquity = Angle (25.19 * pi / 180.0)
  , totalPressure = Pressure 0.130
  , slopeAngleNorth = Angle (30.0 * pi / 180.0)
  , slopeAngleSouth = Angle (-30.0 * pi / 180.0)
  }

-- | Initialize climate state
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
