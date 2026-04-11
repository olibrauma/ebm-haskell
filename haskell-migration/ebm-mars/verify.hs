import Verification
import Control.Monad (forM_)
import Text.Printf (printf)
import System.Exit (exitFailure, exitSuccess)

main :: IO ()
main = do
  putStrLn "Verifying dump_000.dat to dump_010.dat against golden-master..."
  results <- forM [0..10] $ \i -> do
    let filename = printf "dump_%03d.dat" (i :: Int)
    let golden = "golden-master/standard/" ++ filename
    let test = "test_output/" ++ filename
    res <- verifyDumpFile golden test
    if passed res
      then do
        putStrLn $ filename ++ " PASSED (Max Rel Err: " ++ show (maxRelativeError res) ++ ")"
        return True
      else do
        putStrLn $ filename ++ " FAILED"
        mapM_ (\(name, absE, relE) -> 
          putStrLn $ "  - " ++ name ++ " (Abs: " ++ show absE ++ ", Rel: " ++ show relE ++ ")") (failedChecks res)
        return False
        
  if and results
    then putStrLn "✓ ALL TESTS PASSED." >> exitSuccess
    else putStrLn "✗ SOME TESTS FAILED." >> exitFailure

-- Support 'forM' mapping locally since importing Control.Monad without knowing standard Prelude version might be buggy
forM :: Monad m => [a] -> (a -> m b) -> m [b]
forM [] _ = return []
forM (x:xs) f = do
  y <- f x
  ys <- forM xs f
  return (y:ys)
