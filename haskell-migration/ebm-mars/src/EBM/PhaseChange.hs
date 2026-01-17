{-|
Module      : EBM.PhaseChange
Description : Phase change processes (CO2 ice/regolith dynamics)
Copyright   : (c) 2026
License     : MIT

Functions for calculating CO2 phase changes, including sublimation,
ice formation, and regolith adsorption on Mars.
-}

module EBM.PhaseChange
  ( SublimationParams(..)
  , calcSublimationParams
  , calcTemperatureMass
  , calcSlopeTemperatureMass
  , calcIcePressure
  , calcRegolithPressure
  ) where

import EBM.Types
import qualified Data.Vector.Unboxed as V

-- Physical constants
latentHeat :: Double
latentHeat = 5.9e5  -- Latent heat of CO2 [J/kg]

heatCapacity :: Double
heatCapacity = 1.0e7  -- Heat capacity [J/K/m^2]

iceMassFactor :: Double
iceMassFactor = 5.815e-6  -- Conversion factor [bar/(kg/m^2)]

-- | Sublimation parameters (temperature and albedos)
data SublimationParams = SublimationParams
  { sublimationTemperature :: !Double  -- [K]
  , albedoFrost :: !Double             -- Frost albedo
  , albedoIce :: !Double               -- Ice albedo
  } deriving (Show, Eq)

-- | Calculate sublimation parameters from pressure
calcSublimationParams :: Pressure -> SublimationParams
calcSublimationParams (Pressure pAir) =
  let -- Polynomial coefficients for sublimation temperature
      tCoeffs = [194.36, 26.451, 2.8593, 0.1814, 0.0046]
      
      -- Polynomial coefficients for frost albedo
      afCoeffs = [0.21, -0.0008, -0.0074, -0.0147, 0.0337, 0.1381, 0.3249]
      
      -- Polynomial coefficients for ice albedo
      aiCoeffs = [0.63, -0.0008, -0.0011, 0.0183, 0.0599, 0.6997]
      
      x = if pAir >= 1e-16
          then log pAir / log 10.0  -- log10
          else -16.0
      
      -- Calculate sublimation temperature
      tsub = sum $ zipWith (\c i -> c * x ** fromIntegral i) tCoeffs [0..]
      
      -- Calculate albedos
      (af, ai) = if pAir >= 1e-3
                 then let af' = afCoeffs !! 1 * x**5 + afCoeffs !! 2 * x**4 + 
                               afCoeffs !! 3 * x**3 + afCoeffs !! 4 * x**2 + 
                               afCoeffs !! 5 * x + afCoeffs !! 6
                          ai' = aiCoeffs !! 1 * x**4 + aiCoeffs !! 2 * x**3 + 
                               aiCoeffs !! 3 * x**2 + aiCoeffs !! 4 * x + 
                               aiCoeffs !! 5 - 0.08
                      in (af', ai')
                 else (afCoeffs !! 0, aiCoeffs !! 0 - 0.08)
      
  in SublimationParams tsub af ai

-- | Calculate temperature and mass evolution (global)
calcTemperatureMass :: SimulationConfig -> PlanetParams -> ClimateState 
                    -> V.Vector Double -> V.Vector Double -> V.Vector Double
                    -> ClimateState
calcTemperatureMass config params state ins dif radi =
  let reso = resolution config
      dt = unTime $ timeStep config
      l = latentHeat
      c = heatCapacity
      
      subParams = calcSublimationParams (airPressure state)
      tsub = sublimationTemperature subParams
      af = albedoFrost subParams
      ai = albedoIce subParams
      
      temps = temperatures state
      masses = co2Masses state
      
      -- Update temperature and mass for each latitude
      (newTemps, newMasses) = V.unzip $ V.generate reso $ \n ->
        let t = temps V.! n
            m = masses V.! n
            insN = ins V.! n
            difN = dif V.! n
            radiN = radi V.! n
        in if m == 0.0
           then -- No ice present
             let delE = (insN * (1.0 - af) + difN - radiN) * dt
                 tNew = t + delE / c
             in if tNew < tsub
                then (tsub, (tsub - tNew) * c / l)  -- Ice forms
                else (tNew, 0.0)
           else -- Ice present
             let delE = (insN * (1.0 - ai) + difN - radiN) * dt
                 mNew = m - delE / l + (tsub - t) * c / l
             in if mNew < 0.0
                then (tsub + (-mNew) * l / c, 0.0)  -- Ice sublimates
                else (tsub, mNew)
      
  in state { temperatures = newTemps
           , co2Masses = newMasses
           , sublimationTemp = Temperature tsub
           }

-- | Calculate temperature and mass evolution (slope)
calcSlopeTemperatureMass :: SimulationConfig -> PlanetParams -> ClimateState
                         -> V.Vector Double -> V.Vector Double  -- Current T and M
                         -> V.Vector Double -> V.Vector Double -> V.Vector Double  -- ins, radi, dif
                         -> (V.Vector Double, V.Vector Double)  -- New T and M
calcSlopeTemperatureMass config params state slopeT slopeM ins radi dif =
  let reso = resolution config
      dt = unTime $ timeStep config
      l = latentHeat
      c = heatCapacity
      
      subParams = calcSublimationParams (airPressure state)
      tsub = sublimationTemperature subParams
      af = albedoFrost subParams
      ai = albedoIce subParams
      
      -- Update temperature and mass for each latitude
      V.unzip $ V.generate reso $ \n ->
        let t = slopeT V.! n
            m = slopeM V.! n
            insN = ins V.! n
            difN = dif V.! n
            radiN = radi V.! n
        in if m == 0.0
           then -- No ice present
             let delE = (insN * (1.0 - af) + difN - radiN) * dt
                 tNew = t + delE / c
             in if tNew < tsub
                then (tsub, (tsub - tNew) * c / l)  -- Ice forms
                else (tNew, 0.0)
           else -- Ice present
             let delE = (insN * (1.0 - ai) + difN - radiN) * dt
                 mNew = m - delE / l + (tsub - t) * c / l
             in if mNew < 0.0
                then (tsub + (-mNew) * l / c, 0.0)  -- Ice sublimates
                else (tsub, mNew)

-- | Calculate ice pressure from CO2 mass distribution
calcIcePressure :: SimulationConfig -> PlanetParams -> ClimateState -> Pressure
calcIcePressure config params state =
  let reso = resolution config
      masses = co2Masses state
      factor = iceMassFactor
      
      -- Integrate over latitude bands
      pice = V.sum $ V.generate reso $ \n ->
        let upper = 90.0 / 181.0 * (2.0 * fromIntegral n - 179.0) * pi / 180.0
            lower = 90.0 / 181.0 * (2.0 * fromIntegral n - 181.0) * pi / 180.0
            scale = 2.0 * pi * (sin upper - sin lower)
        in masses V.! n * factor * scale
      
  in if pice > unPressure (totalPressure params)
     then totalPressure params
     else Pressure pice

-- | Calculate regolith pressure (adsorbed CO2)
calcRegolithPressure :: SimulationConfig -> PlanetParams -> ClimateState -> (Pressure, Pressure, Double)
calcRegolithPressure config params state =
  let reso = resolution config
      cConst = 34.0
      tD = 35.0
      gamma = 0.275
      
      temps = temperatures state
      tsub = unTemperature $ sublimationTemp state
      pIce = unPressure $ icePressure state
      pTotal = unPressure $ totalPressure params
      
      -- Calculate sigma (regolith capacity)
      sigma = V.sum $ V.generate reso $ \n ->
        let theta = fromIntegral (n - 90) * pi / 180.0
            t = temps V.! n
        in if t > tsub
           then cConst * exp (-t / tD) * cos theta * (pi / 180.0)
           else 0.0
      
      -- Solve for atmospheric pressure using bisection
      (pAir, pRego, bug) = solvePressure sigma pIce pTotal gamma 0 100
      
  in (Pressure pAir, Pressure pRego, bug)
  where
    -- Bisection solver for pressure balance equation
    solvePressure :: Double -> Double -> Double -> Double -> Int -> Int -> (Double, Double, Double)
    solvePressure sigma pIce pTotal gamma loop loopMax =
      let kouho0 = pTotal / 2.0
          limHi0 = pTotal
          limLo0 = 0.0
      in go kouho0 limHi0 limLo0 loop
      where
        go kouho limHi limLo loopCount
          | loopCount >= loopMax = (kouho, sigma * kouho ** gamma, 0.0)
          | otherwise =
              let fx = sigma * kouho ** gamma + kouho + pIce - pTotal
              in if fx == 0.0
                 then (kouho, sigma * kouho ** gamma, if kouho < 0 then 1.0 else 0.0)
                 else if fx > 0.0
                      then let kouhoNew = 0.5 * (kouho + limLo)
                           in go kouhoNew kouho limLo (loopCount + 1)
                      else let kouhoNew = 0.5 * (limHi + kouho)
                           in go kouhoNew limHi kouho (loopCount + 1)
