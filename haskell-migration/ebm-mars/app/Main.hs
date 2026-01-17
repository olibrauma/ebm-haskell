module Main (main) where

import EBM.Types
import EBM.Config
import EBM.IO

main :: IO ()
main = do
  putStrLn "EBM-Mars Haskell Implementation"
  putStrLn "Phase 1: Foundation Setup Complete"
  putStrLn ""
  putStrLn "Next steps:"
  putStrLn "  - Implement Insolation module"
  putStrLn "  - Implement Atmosphere module"
  putStrLn "  - Implement PhaseChange module"
  putStrLn "  - Implement Simulation module"
  
  -- Test basic functionality
  let config = defaultConfig
      params = defaultParams
      state = initializeState config params
  
  putStrLn $ "\nInitial state created with " ++ show (resolution config) ++ " latitude points"
  putStrLn $ "Initial temperature: " ++ show (temperatures state V.! 90) ++ " K"
