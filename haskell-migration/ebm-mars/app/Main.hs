-- |
-- Module      : Main
-- Description : Main entry point for EBM-Mars simulation
-- Copyright   : (c) 2026
-- License     : MIT
--
-- Main program that runs the Mars Energy Balance Model simulation.
-- Supports command-line arguments for pressure, obliquity, and slope angle.
module Main (main) where

import Data.Vector.Unboxed qualified as V
import EBM.Config
import EBM.IO
import EBM.Insolation (calcYearLength)
import EBM.Simulation
import EBM.Types
import System.Environment (getArgs)
import System.IO (hPutStrLn, stderr)
import Text.Printf (printf)

main :: IO ()
main = do
  args <- getArgs

  -- Parse command-line arguments
  let (config, params, outDir) = parseCommandLine args

  -- Initialize state
  let initialState = initializeState config params

  -- Run equilibrium calculation
  -- Run equilibrium calculation
  -- hPutStrLn stderr "Skipping equilibrium state for debugging..."
  let eqState = equilibrium config params initialState
  -- let eqState = initialState

  -- Dump initial state
  dumpState outDir "dump_000.dat" eqState

  -- Run time evolution if no error
  if bugFlag eqState == 0.0
    then runTimeEvolution config params eqState outDir
    else hPutStrLn stderr "Error: Bug flag set"

-- | Parse command-line arguments
parseCommandLine :: [String] -> (SimulationConfig, PlanetParams, FilePath)
parseCommandLine args =
  let config = defaultConfig {yearLength = calcYearLength}

      params = case args of
        (pStr : oblStr : alphaStr : _) ->
          let p = read pStr :: Double
              obl = read oblStr :: Double
              alpha = read alphaStr :: Double
           in defaultParams
                { totalPressure = Pressure p,
                  obliquity = Angle (obl * pi / 180.0),
                  slopeAngleNorth = Angle (alpha * pi / 180.0),
                  slopeAngleSouth = Angle (-alpha * pi / 180.0)
                }
        _ -> defaultParams

      outDir = case args of
        (_ : _ : _ : dir : _) -> dir
        _ -> "."
   in (config, params, outDir)

-- | Run time evolution for specified number of steps
runTimeEvolution :: SimulationConfig -> PlanetParams -> ClimateState -> FilePath -> IO ()
runTimeEvolution config params initialState outDir =
  go initialState 1
  where
    go state stepCount
      | stepCount > stepLimit config = return ()
      | bugFlag state /= 0.0 = do
          hPutStrLn stderr $ "Error: Bug flag set at step " ++ show stepCount
          return ()
      | otherwise = do
          -- Perform one time step
          let newState = stepClimate config params state

          -- Dump state
          let filename = printf "dump_%03d.dat" stepCount
          dumpState outDir filename newState

          -- Check if we've completed one year
          let yearSec = unTime $ yearLength config
              currentLoop = loopCount newState
              targetSeason = yearSec * fromIntegral (currentLoop + 1)

          if unTime (season newState) >= targetSeason
            then return () -- Completed one year
            else go newState (stepCount + 1)
