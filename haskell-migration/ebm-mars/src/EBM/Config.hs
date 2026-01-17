{-|
Module      : EBM.Config
Description : Configuration utilities
Copyright   : (c) 2026
License     : MIT

Utilities for parsing command-line arguments and creating
configuration from user input.
-}

module EBM.Config
  ( parseArgs
  , configFromArgs
  ) where

import EBM.Types
import Options.Applicative

-- | Command-line arguments
data Args = Args
  { argPressure :: Maybe Double
  , argObliquity :: Maybe Double
  , argSlope :: Maybe Double
  , argOutputDir :: Maybe FilePath
  } deriving (Show)

-- | Parse command-line arguments
parseArgs :: IO Args
parseArgs = execParser opts
  where
    opts = info (argsParser <**> helper)
      ( fullDesc
     <> progDesc "Run EBM simulation for Mars"
     <> header "ebm-mars - Energy Balance Model for Mars" )

-- | Argument parser
argsParser :: Parser Args
argsParser = Args
  <$> optional (argument auto
      ( metavar "PRESSURE"
     <> help "Total pressure [bar]" ))
  <*> optional (argument auto
      ( metavar "OBLIQUITY"
     <> help "Obliquity [degrees]" ))
  <*> optional (argument auto
      ( metavar "SLOPE"
     <> help "Slope angle [degrees]" ))
  <*> optional (argument str
      ( metavar "OUTPUT_DIR"
     <> help "Output directory" ))

-- | Create configuration from parsed arguments
configFromArgs :: Args -> (SimulationConfig, PlanetParams, FilePath)
configFromArgs args =
  let config = defaultConfig
      params = defaultParams
        { totalPressure = maybe (totalPressure defaultParams) Pressure (argPressure args)
        , obliquity = maybe (obliquity defaultParams) (\o -> Angle (o * pi / 180.0)) (argObliquity args)
        , slopeAngleNorth = maybe (slopeAngleNorth defaultParams) (\a -> Angle (a * pi / 180.0)) (argSlope args)
        , slopeAngleSouth = maybe (slopeAngleSouth defaultParams) (\a -> Angle (-a * pi / 180.0)) (argSlope args)
        }
      outDir = maybe "." id (argOutputDir args)
  in (config, params, outDir)
