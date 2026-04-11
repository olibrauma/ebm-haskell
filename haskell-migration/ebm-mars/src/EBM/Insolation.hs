{-|
Module      : EBM.Insolation
Description : Insolation (solar radiation) calculations
Copyright   : (c) 2026
License     : MIT

Functions for calculating solar insolation on Mars, including
orbital mechanics and slope-dependent radiation.
-}

module EBM.Insolation
  ( calcYearLength
  , calcDelta
  , calcInsolation
  , calcSlopeInsolation
  ) where

import EBM.Types
import qualified Data.Vector.Unboxed as V

-- Physical constants
semiMajorAxis :: Double
semiMajorAxis = 227936640000.0  -- [m]

gravitationalConstant :: Double
gravitationalConstant = 6.67384e-11  -- [m^3 kg^-1 s^-2]

sunMass :: Double
sunMass = 1.9891e30  -- [kg]

marsMass :: Double
marsMass = 639e21  -- [kg]

eccentricity :: Double
eccentricity = 0.0934

perihelion :: Double
perihelion = 336.049 * pi / 180.0  -- [rad]

oneAU :: Double
oneAU = 149597871000.0  -- [m]

-- | Calculate Martian year length [s]
calcYearLength :: Time
calcYearLength =
  let ma = semiMajorAxis
      g = gravitationalConstant
      m = sunMass
      mm = marsMass
  in Time $ sqrt (ma * ma * ma / (g * (m + mm))) * 2.0 * pi

-- | Calculate orbital parameters (declination and distance)
-- Returns (delta [rad], r_AU)
calcDelta :: SimulationConfig -> PlanetParams -> Time -> (Angle, Double)
calcDelta config params season =
  let ma = semiMajorAxis
      g = gravitationalConstant
      m = sunMass
      mm = marsMass
      ecc = eccentricity
      p = perihelion
      
      nt = sqrt (g * (m + mm) / ma / ma / ma) * unTime season
      
      -- Solve Kepler's equation iteratively
      u = if ecc == 0.0
          then nt
          else let x0 = nt
                   x1 = x0 - (x0 - ecc * sin x0 - nt) / (1.0 - ecc * cos x0)
               in solveKepler ecc nt x1 1 100
      
      r = ma * (1.0 - ecc * cos u)
      rAU = r / oneAU
      
      -- Calculate true anomaly
      (cosf, sinf) = if ecc /= 0.0
                     then let cf = (ma * (1.0 - ecc * ecc) / r - 1.0) / ecc
                              sf = sqrt (1.0 - cf * cf)
                          in if sin u < 0.0
                             then (cf, -sf)
                             else (cf, sf)
                     else (cos u, sin u)
      
      -- Calculate declination
      delta = asin (sin (unAngle $ obliquity params) * (sinf * cos p + cosf * sin p))
      
  in (Angle delta, rAU)
  where
    -- Solve Kepler's equation: x - ecc * sin(x) - nt = 0
    -- CRITICAL: Match C version exactly (no fabs in condition)
    solveKepler :: Double -> Double -> Double -> Int -> Int -> Double
    solveKepler ecc nt x loop loopMax
      | loop >= loopMax = x
      | x - ecc * sin x - nt >= 1.0e-6 = 
          let xNew = x - (x - ecc * sin x - nt) / (1.0 - ecc * cos x)
          in solveKepler ecc nt xNew (loop + 1) loopMax
      | otherwise = x

-- | Floating-point modulo (C fmod equivalent)
fmod :: Double -> Double -> Double
fmod x y = x - fromIntegral (floor (x / y)) * y

-- | Calculate global insolation
calcInsolation :: SimulationConfig -> PlanetParams -> ClimateState -> V.Vector Double
calcInsolation config params state =
  let reso = resolution config
      q = solarConstant params
      theta = V.generate reso (\n -> (fromIntegral n - 90.0) * pi / 180.0)
      
      -- Calculate hour angle h (matching C version exactly)
      h = (unTime (season state) `fmod` unTime (dayLength config)) * 2.0 * pi / unTime (dayLength config) - pi
      
      (delta, rAU) = calcDelta config params (season state)
      
  in V.generate reso $ \n ->
       let th = theta V.! n
           cosH = -tan th * tan (unAngle delta)
           hAngle = if cosH >= 1.0
                    then 0.0
                    else if cosH <= -1.0
                    then pi
                    else acos cosH
       in if rAU /= 0.0
          then (q / pi / rAU / rAU) * 
               (hAngle * sin th * sin (unAngle delta) + 
                cos th * cos (unAngle delta) * sin hAngle)
          else 0.0

-- | Calculate slope insolation
calcSlopeInsolation :: SimulationConfig -> PlanetParams -> ClimateState -> Angle -> V.Vector Double
calcSlopeInsolation config params state alpha =
  let reso = resolution config
      q = solarConstant params
      
      (delta, rAU) = calcDelta config params (season state)
      h = (unTime (season state) `fmod` unTime (dayLength config)) * 2.0 * pi / unTime (dayLength config) - pi
      
      theta = V.generate reso (\n -> (fromIntegral n - 90.0) * pi / 180.0)
      
  in V.generate reso $ \n ->
       let th = theta V.! n
           slope0 = th + unAngle alpha
           
           cosHt = -tan th * tan (unAngle delta)
           hT = if cosHt >= 1.0
                then 0.0
                else if cosHt <= -1.0
                then pi
                else acos cosHt
           
           -- Adjust slope angle if out of range
           (slope, mark) = if slope0 > 0.5 * pi
                           then (pi - slope0, 1)
                           else if slope0 < -0.5 * pi
                           then (-pi - slope0, 2)
                           else (slope0, 0)
           
           cosHs = -tan slope * tan (unAngle delta)
           hS = if cosHs >= 1.0
                then 0.0
                else if cosHs <= -1.0
                then pi
                else acos cosHs
           
           -- Restore slope if it was adjusted
           slopeFinal = case mark of
                          1 -> pi - slope
                          2 -> -pi - slope
                          _ -> slope
           
           hEff = max hT hS  -- C version uses fmax
           
       in if h >= -hEff && h <= hEff
          then let ins = q / rAU / rAU *  -- C version: no division by pi
                         (sin slopeFinal * sin (unAngle delta) + 
                          cos slopeFinal * cos (unAngle delta) * cos h)
               in if ins <= 0.0 then 0.0 else ins
          else 0.0
