{-|
Module      : EBM.IO
Description : Input/Output utilities
Copyright   : (c) 2026
License     : MIT

Functions for dumping simulation state to files in the format
compatible with the C implementation.
-}

module EBM.IO
  ( dumpState
  ) where

import EBM.Types
import qualified Data.Vector.Unboxed as V
import System.FilePath ((</>))
import Text.Printf (printf)

-- | Dump climate state to file (C-compatible format)
dumpState :: FilePath -> String -> ClimateState -> IO ()
dumpState dir filename state = do
  let path = dir </> filename
  writeFile path $ formatState state

-- | Format state as string (matching C output format)
formatState :: ClimateState -> String
formatState state =
  let header = printf "loop:%d,season:%g,P_air:%g,P_ice:%g,P_rego:%g,T_sub:%g\n"
        (loopCount state)
        (unTime $ season state)
        (unPressure $ airPressure state)
        (unPressure $ icePressure state)
        (unPressure $ regolithPressure state)
        (unTemperature $ sublimationTemp state)
      dataLines = V.toList $ V.imap formatDataLine (temperatures state)
  in header ++ concat dataLines
  where
    formatDataLine :: Int -> Double -> String
    formatDataLine i temp =
      let mass = co2Masses state V.! i
          tPosi = tempNorthSlope state V.! i
          mPosi = massNorthSlope state V.! i
          tNega = tempSouthSlope state V.! i
          mNega = massSouthSlope state V.! i
      in printf "%d,%.16g,%.16g,%.16g,%.16g,%.16g,%.16g\n" i temp mass tPosi mPosi tNega mNega
