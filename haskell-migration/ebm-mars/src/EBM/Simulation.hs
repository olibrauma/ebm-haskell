-- |
-- Module      : EBM.Simulation
-- Description : Simulation control (equilibrium and time evolution)
-- Copyright   : (c) 2026
-- License     : MIT
--
-- High-level simulation functions including equilibrium calculation
-- and time-stepping for the Mars climate model.
module EBM.Simulation
  ( equilibrium,
    stepClimate,
  )
where

import Data.Vector.Unboxed qualified as V
import EBM.Atmosphere
import EBM.Insolation
import EBM.PhaseChange
import EBM.Types
import System.IO (hPutStrLn, stderr)

-- | Calculate equilibrium state (heikou function from C)
equilibrium :: SimulationConfig -> PlanetParams -> ClimateState -> ClimateState
equilibrium config params initialState =
  let reso = resolution config
      yearSec = unTime $ yearLength config

      -- Initialize last temperature arrays for convergence check
      initLastT = temperatures initialState
      initLastTPosi = tempNorthSlope initialState
      initLastTNega = tempSouthSlope initialState
   in go initialState initLastT initLastTPosi initLastTNega 0 0.0
  where
    go state lastT lastTPosi lastTNega loop loopEnd
      | bugFlag state /= 0.0 = state -- Error condition
      | loopEnd /= 0.0 = state {loopCount = loop} -- Converged
      | loop >= 1 = state {loopCount = loop} -- Max loops reached
      | otherwise =
          let yearSec = unTime $ yearLength config
              targetSeason = yearSec * fromIntegral (loop + 1)

              -- Run one year
              stateAfterYear = runOneYear config params state targetSeason

              -- Check convergence
              reso = resolution config
              temps = temperatures stateAfterYear
              tempsPosi = tempNorthSlope stateAfterYear
              tempsNega = tempSouthSlope stateAfterYear

              -- Calculate RMS differences
              absGlobal = sqrt $ V.sum $ V.zipWith (\t1 t2 -> (t1 - t2) ** 2) lastT temps
              absPosi = sqrt $ V.sum $ V.zipWith (\t1 t2 -> (t1 - t2) ** 2) lastTPosi tempsPosi
              absNega = sqrt $ V.sum $ V.zipWith (\t1 t2 -> (t1 - t2) ** 2) lastTNega tempsNega

              converged =
                if absGlobal < 1.0 && absPosi < 1.0 && absNega < 1.0
                  then 1.0
                  else 0.0
           in go stateAfterYear temps tempsPosi tempsNega (loop + 1) converged

    -- Run simulation for one Martian year
    runOneYear :: SimulationConfig -> PlanetParams -> ClimateState -> Double -> ClimateState
    runOneYear config params state targetSeason =
      if unTime (season state) >= targetSeason || bugFlag state /= 0.0
        then state
        else
          -- Perform one time step
          let newState = stepClimate config params state
           in runOneYear config params newState targetSeason

-- | Perform one time step
stepClimate :: SimulationConfig -> PlanetParams -> ClimateState -> ClimateState
stepClimate config params state =
  let reso = resolution config

      -- Calculate global physics
      ins = calcInsolation config params state
      dif = calcDiffusion config params state
      radi = calcRadiation config params state

      -- Update global temperature and mass
      state1 = calcTemperatureMass config params state ins dif radi

      -- Calculate ice and regolith pressures
      pIce = calcIcePressure config params state1
      (pAir, pRego, bug) = calcRegolithPressure config params state1 {icePressure = pIce}

      state2 =
        state1
          { icePressure = pIce,
            airPressure = pAir,
            regolithPressure = pRego,
            bugFlag = bug
          }

      -- Calculate north slope physics
      insPosi = calcSlopeInsolation config params state2 (slopeAngleNorth params)
      radiPosi = calcSlopeRadiation config params state2 (tempNorthSlope state2)
      difPosi = calcSlopeDiffusion config params state2 (tempNorthSlope state2)
      (newTPosi, newMPosi) =
        calcSlopeTemperatureMass
          config
          params
          state2
          (tempNorthSlope state2)
          (massNorthSlope state2)
          insPosi
          radiPosi
          difPosi

      -- Calculate south slope physics
      insNega = calcSlopeInsolation config params state2 (slopeAngleSouth params)
      radiNega = calcSlopeRadiation config params state2 (tempSouthSlope state2)
      difNega = calcSlopeDiffusion config params state2 (tempSouthSlope state2)
      (newTNega, newMNega) =
        calcSlopeTemperatureMass
          config
          params
          state2
          (tempSouthSlope state2)
          (massSouthSlope state2)
          insNega
          radiNega
          difNega

      -- Update season
      newSeason = season state2 + EBM.Types.timeStep config
   in state2
        { tempNorthSlope = newTPosi,
          massNorthSlope = newMPosi,
          tempSouthSlope = newTNega,
          massSouthSlope = newMNega,
          season = newSeason
        }
