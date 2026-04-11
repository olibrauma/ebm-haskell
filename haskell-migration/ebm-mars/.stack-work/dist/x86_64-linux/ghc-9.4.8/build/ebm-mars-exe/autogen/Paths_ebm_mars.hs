{-# LANGUAGE CPP #-}
{-# LANGUAGE NoRebindableSyntax #-}
{-# OPTIONS_GHC -fno-warn-missing-import-lists #-}
{-# OPTIONS_GHC -w #-}
module Paths_ebm_mars (
    version,
    getBinDir, getLibDir, getDynLibDir, getDataDir, getLibexecDir,
    getDataFileName, getSysconfDir
  ) where


import qualified Control.Exception as Exception
import qualified Data.List as List
import Data.Version (Version(..))
import System.Environment (getEnv)
import Prelude


#if defined(VERSION_base)

#if MIN_VERSION_base(4,0,0)
catchIO :: IO a -> (Exception.IOException -> IO a) -> IO a
#else
catchIO :: IO a -> (Exception.Exception -> IO a) -> IO a
#endif

#else
catchIO :: IO a -> (Exception.IOException -> IO a) -> IO a
#endif
catchIO = Exception.catch

version :: Version
version = Version [0,1,0,0] []

getDataFileName :: FilePath -> IO FilePath
getDataFileName name = do
  dir <- getDataDir
  return (dir `joinFileName` name)

getBinDir, getLibDir, getDynLibDir, getDataDir, getLibexecDir, getSysconfDir :: IO FilePath



bindir, libdir, dynlibdir, datadir, libexecdir, sysconfdir :: FilePath
bindir     = "/home/takeru/git/EBM-on-Mars/haskell-migration/ebm-mars/.stack-work/install/x86_64-linux/adbb68db0091794eff7db37725d73be34e1dd483fcbaa424fe33903096ef0623/9.4.8/bin"
libdir     = "/home/takeru/git/EBM-on-Mars/haskell-migration/ebm-mars/.stack-work/install/x86_64-linux/adbb68db0091794eff7db37725d73be34e1dd483fcbaa424fe33903096ef0623/9.4.8/lib/x86_64-linux-ghc-9.4.8/ebm-mars-0.1.0.0-INM9czlNlKsES0kiDGq92h-ebm-mars-exe"
dynlibdir  = "/home/takeru/git/EBM-on-Mars/haskell-migration/ebm-mars/.stack-work/install/x86_64-linux/adbb68db0091794eff7db37725d73be34e1dd483fcbaa424fe33903096ef0623/9.4.8/lib/x86_64-linux-ghc-9.4.8"
datadir    = "/home/takeru/git/EBM-on-Mars/haskell-migration/ebm-mars/.stack-work/install/x86_64-linux/adbb68db0091794eff7db37725d73be34e1dd483fcbaa424fe33903096ef0623/9.4.8/share/x86_64-linux-ghc-9.4.8/ebm-mars-0.1.0.0"
libexecdir = "/home/takeru/git/EBM-on-Mars/haskell-migration/ebm-mars/.stack-work/install/x86_64-linux/adbb68db0091794eff7db37725d73be34e1dd483fcbaa424fe33903096ef0623/9.4.8/libexec/x86_64-linux-ghc-9.4.8/ebm-mars-0.1.0.0"
sysconfdir = "/home/takeru/git/EBM-on-Mars/haskell-migration/ebm-mars/.stack-work/install/x86_64-linux/adbb68db0091794eff7db37725d73be34e1dd483fcbaa424fe33903096ef0623/9.4.8/etc"

getBinDir     = catchIO (getEnv "ebm_mars_bindir")     (\_ -> return bindir)
getLibDir     = catchIO (getEnv "ebm_mars_libdir")     (\_ -> return libdir)
getDynLibDir  = catchIO (getEnv "ebm_mars_dynlibdir")  (\_ -> return dynlibdir)
getDataDir    = catchIO (getEnv "ebm_mars_datadir")    (\_ -> return datadir)
getLibexecDir = catchIO (getEnv "ebm_mars_libexecdir") (\_ -> return libexecdir)
getSysconfDir = catchIO (getEnv "ebm_mars_sysconfdir") (\_ -> return sysconfdir)




joinFileName :: String -> String -> FilePath
joinFileName ""  fname = fname
joinFileName "." fname = fname
joinFileName dir ""    = dir
joinFileName dir fname
  | isPathSeparator (List.last dir) = dir ++ fname
  | otherwise                       = dir ++ pathSeparator : fname

pathSeparator :: Char
pathSeparator = '/'

isPathSeparator :: Char -> Bool
isPathSeparator c = c == '/'
