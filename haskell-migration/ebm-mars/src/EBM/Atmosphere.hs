-- |
-- Module      : EBM.Atmosphere
-- Description : Atmospheric processes (diffusion and radiation)
-- Copyright   : (c) 2026
-- License     : MIT
--
-- Functions for calculating atmospheric diffusion and radiative transfer
-- on Mars, including both global and slope-specific calculations.
module EBM.Atmosphere
  ( calcDiffusion,
    calcRadiation,
    calcSlopeDiffusion,
    calcSlopeRadiation,
  )
where

import Data.Vector.Unboxed qualified as V
import EBM.Types

-- | Diffusion coefficient [bar^-1]
diffusionCoefficient :: Double
diffusionCoefficient = 5.3e-3

-- | Calculate atmospheric diffusion
calcDiffusion :: SimulationConfig -> PlanetParams -> ClimateState -> V.Vector Double
calcDiffusion config params state =
  let reso = resolution config
      d = diffusionCoefficient * unPressure (airPressure state)
      delPhi = pi / 180.0

      phi = V.generate reso (\n -> fromIntegral (n - 90) * pi / 180.0)
      temps = temperatures state
   in V.generate reso $ \n ->
        if n == 0 || n == reso - 1
          then 0.0 -- Boundary conditions
          else
            let phiN = phi V.! n
                tNext = temps V.! (n + 1)
                tPrev = temps V.! (n - 1)
                tCurr = temps V.! n
             in -d * tan phiN * (tNext - tPrev) * 0.5 / delPhi
                  + d * (tNext + tPrev - 2.0 * tCurr) / delPhi / delPhi

-- | Radiation coefficients for high temperature (T > 230.1 K)
highTempCoeffs :: ([Double], [Double])
highTempCoeffs =
  ( [-372.7, 329.9, 99.54, 13.28, 0.6449], -- a coefficients
    [1.898, -1.68, -0.5069, -0.06758, -0.003256] -- b coefficients
  )

-- | Radiation coefficients for low temperature (T <= 230.1 K)
lowTempCoeffs :: ([Double], [Double])
lowTempCoeffs =
  ( [-61.72, 54.64, 16.48, 2.198, 0.1068], -- a coefficients
    [0.5479, -0.485, -0.1464, -0.0195, -0.00094] -- b coefficients
  )

-- | Calculate radiation using polynomial coefficients
calcRadiationValue :: Double -> Double -> Double
calcRadiationValue pAir temp =
  let -- Clamp pressure to valid range
      pClamped =
        if pAir > 7.45
          then 7.45
          else
            if pAir < 3.4e-12
              then 3.4e-12
              else pAir

      x = log pClamped / log 10.0 -- log10

      -- Select coefficients based on temperature
      (aCoeffs, bCoeffs) =
        if temp > 230.1
          then highTempCoeffs
          else lowTempCoeffs

      -- Calculate A and B using polynomial
      calcPoly coeffs = sum $ zipWith (\c i -> c * x ** fromIntegral i) coeffs [0 ..]

      a = calcPoly aCoeffs
      b = calcPoly bCoeffs

      radi = a + b * temp
   in if radi <= 0.0 then 0.0 else radi

-- | Calculate atmospheric radiation
calcRadiation :: SimulationConfig -> PlanetParams -> ClimateState -> V.Vector Double
calcRadiation config params state =
  let reso = resolution config
      pAir = unPressure $ airPressure state
      temps = temperatures state
   in V.map (calcRadiationValue pAir) temps

-- | Calculate slope radiation
calcSlopeRadiation :: SimulationConfig -> PlanetParams -> ClimateState -> V.Vector Double -> V.Vector Double
calcSlopeRadiation config params state slopeTemps =
  let pAir = unPressure $ airPressure state
   in V.map (calcRadiationValue pAir) slopeTemps

-- | Calculate slope diffusion (heat exchange with atmosphere)
calcSlopeDiffusion :: SimulationConfig -> PlanetParams -> ClimateState -> V.Vector Double -> V.Vector Double
calcSlopeDiffusion config params state slopeTemps =
  let reso = resolution config
      m = 10.0 -- Exchange coefficient
      temps = temperatures state
   in V.zipWith (\tGlobal tSlope -> m * (tGlobal - tSlope)) temps slopeTemps
