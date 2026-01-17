{-|
Module      : Verification
Description : Verification utilities for comparing against C implementation
Copyright   : (c) 2026
License     : MIT

Functions for verifying Haskell implementation against golden master
data from the C implementation.
-}

module Verification
  ( VerificationResult(..)
  , verifyDumpFile
  , verifyState
  , readDumpFile
  ) where

import EBM.Types
import qualified Data.Vector.Unboxed as V
import System.FilePath ((</>))
import Text.Printf (printf)
import Text.Read (readMaybe)

-- | Verification result
data VerificationResult = VerificationResult
  { passed :: !Bool
  , maxRelativeError :: !Double
  , maxAbsoluteError :: !Double
  , failedChecks :: [(String, Double, Double)]
  } deriving (Show, Eq)

-- | Verify dump file against golden master
verifyDumpFile :: FilePath -> FilePath -> IO VerificationResult
verifyDumpFile goldenPath testPath = do
  golden <- readDumpFile goldenPath
  test <- readDumpFile testPath
  return $ verifyState golden test

-- | Verify two states for equivalence
verifyState :: ClimateState -> ClimateState -> VerificationResult
verifyState golden test =
  let tolerance = 1e-10
      checks = concat
        [ checkScalar "P_air" (unPressure $ airPressure golden) (unPressure $ airPressure test)
        , checkScalar "P_ice" (unPressure $ icePressure golden) (unPressure $ icePressure test)
        , checkScalar "P_rego" (unPressure $ regolithPressure golden) (unPressure $ regolithPressure test)
        , checkScalar "T_sub" (unTemperature $ sublimationTemp golden) (unTemperature $ sublimationTemp test)
        , checkVector "T" (temperatures golden) (temperatures test)
        , checkVector "M" (co2Masses golden) (co2Masses test)
        , checkVector "T_posi" (tempNorthSlope golden) (tempNorthSlope test)
        , checkVector "M_posi" (massNorthSlope golden) (massNorthSlope test)
        , checkVector "T_nega" (tempSouthSlope golden) (tempSouthSlope test)
        , checkVector "M_nega" (massSouthSlope golden) (massSouthSlope test)
        ]
      allPassed = null checks
      maxRelErr = if null checks then 0.0 else maximum $ map (\(_, _, r) -> r) checks
      maxAbsErr = if null checks then 0.0 else maximum $ map (\(_, a, _) -> a) checks
      failures = map (\(n, a, r) -> (n, a, r)) checks
  in VerificationResult allPassed maxRelErr maxAbsErr failures
  where
    tolerance = 1e-10
    
    checkScalar :: String -> Double -> Double -> [(String, Double, Double)]
    checkScalar name g t =
      let absDiff = abs (g - t)
          relDiff = if g /= 0.0 then abs ((g - t) / g) else absDiff
      in if absDiff > tolerance && relDiff > tolerance
         then [(name, absDiff, relDiff)]
         else []
    
    checkVector :: String -> V.Vector Double -> V.Vector Double -> [(String, Double, Double)]
    checkVector name g t =
      let diffs = V.zipWith (\gv tv -> 
            let absDiff = abs (gv - tv)
                relDiff = if gv /= 0.0 then abs ((gv - tv) / gv) else absDiff
            in (absDiff, relDiff)) g t
          failures = V.ifilter (\_ (a, r) -> a > tolerance && r > tolerance) diffs
      in if V.null failures
         then []
         else V.toList $ V.imap (\i (a, r) -> (name ++ "[" ++ show i ++ "]", a, r)) failures

-- | Read dump file (C format)
readDumpFile :: FilePath -> IO ClimateState
readDumpFile path = do
  content <- readFile path
  let (headerLine:dataLines) = lines content
      header = parseHeader headerLine
      dataVecs = parseDataLines dataLines
  return $ stateFromParsed header dataVecs

-- | Parse header line
parseHeader :: String -> (Int, Double, Double, Double, Double, Double)
parseHeader line =
  let parts = map (drop 1 . dropWhile (/= ':')) $ words $ map (\c -> if c == ',' then ' ' else c) line
      [loop, season, pAir, pIce, pRego, tSub] = map read parts
  in (loop, season, pAir, pIce, pRego, tSub)

-- | Parse data lines
parseDataLines :: [String] -> (V.Vector Double, V.Vector Double, V.Vector Double, 
                                V.Vector Double, V.Vector Double, V.Vector Double)
parseDataLines lines =
  let parsed = map parseLine lines
      temps = V.fromList $ map (\(_, t, _, _, _, _, _) -> t) parsed
      masses = V.fromList $ map (\(_, _, m, _, _, _, _) -> m) parsed
      tPosi = V.fromList $ map (\(_, _, _, tp, _, _, _) -> tp) parsed
      mPosi = V.fromList $ map (\(_, _, _, _, mp, _, _) -> mp) parsed
      tNega = V.fromList $ map (\(_, _, _, _, _, tn, _) -> tn) parsed
      mNega = V.fromList $ map (\(_, _, _, _, _, _, mn) -> mn) parsed
  in (temps, masses, tPosi, mPosi, tNega, mNega)
  where
    parseLine :: String -> (Int, Double, Double, Double, Double, Double, Double)
    parseLine line =
      let parts = map read $ words $ map (\c -> if c == ',' then ' ' else c) line
      in case parts of
           [i, t, m, tp, mp, tn, mn] -> (i, t, m, tp, mp, tn, mn)
           _ -> error $ "Invalid data line: " ++ line

-- | Construct state from parsed data
stateFromParsed :: (Int, Double, Double, Double, Double, Double)
                -> (V.Vector Double, V.Vector Double, V.Vector Double, 
                    V.Vector Double, V.Vector Double, V.Vector Double)
                -> ClimateState
stateFromParsed (loop, season, pAir, pIce, pRego, tSub) (temps, masses, tPosi, mPosi, tNega, mNega) =
  ClimateState
    { temperatures = temps
    , co2Masses = masses
    , tempNorthSlope = tPosi
    , massNorthSlope = mPosi
    , tempSouthSlope = tNega
    , massSouthSlope = mNega
    , airPressure = Pressure pAir
    , icePressure = Pressure pIce
    , regolithPressure = Pressure pRego
    , season = Time season
    , sublimationTemp = Temperature tSub
    , loopCount = loop
    , bugFlag = 0.0
    }
