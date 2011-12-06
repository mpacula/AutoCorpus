{--
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    Copyright (C) 2011 Maciej Pacula


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
--}

{-# LANGUAGE TypeSynonymInstances #-}

module Main where

import Control.Monad
import Data.List
import Data.Char
import Data.Int
import qualified Data.Map as M
import qualified Data.HashTable as H
import System.Directory
import System.IO
import Test.QuickCheck
import Control.Exception
import Debug.Trace
import Control.Parallel
import System (getArgs)
import System.Console.GetOpt


type CountPair = (String, String, Integer)
type CountTable = H.HashTable (String, String) Integer


{-- TESTS --}

-- instance Arbitrary CountPair where
--   arbitrary = do let words =  ["a", "b", "c", "d", "foo", "bar"]
--                  w <- elements words
--                  v <- elements words
--                  c <- choose (1, 10)
--                  return (w, v, c)
                 
comparePair :: CountPair -> CountPair -> Ordering
(w1,w2,d) `comparePair` (v1,v2,c) = (w1,w2) `compare` (v1,v2)
                  
                   
-- prop_trim_indempotent str = trim str == (trim . trim) str
-- prop_trim_nongrow str = length str >= (length . trim) str


-- prop_sentence_to_set_uniq sentence = (nub (sentenceToSet sentence)) == (sentenceToSet sentence)
-- prop_sentence_to_set_words sentence = sort (nub (words sentence)) == sort (sentenceToSet sentence)


-- prop_counts_sentence sentence context = undefined

-- -- runs all tests
-- test :: IO ()
-- test = mapM_ (\(name, props) -> putStrLn name >> mapM_ quickCheck props)
--            [ ("trim",          [prop_trim_indempotent, prop_trim_nongrow])
--            , ("sentenceToSet", [prop_sentence_to_set_uniq])
--            ]


{-- IMPLEMENTATION --}


serialize :: CountPair -> String
serialize cp@(w,v,c) = show cp

deserialize :: String -> CountPair
deserialize str = read str
                     

-- removes space characters from both sides of a string
trim :: String -> String
trim = trimLeft . trimLeft
       where
         trimLeft = reverse . dropWhile isSpace

sentenceToSet :: String -> [String]
sentenceToSet = nub . words

-- counts colocations for words in a sentence given its context (surrounding sentences)
sentenceCounts :: String -> [String] -> [CountPair]
sentenceCounts sentence context =
  [(a,b,1) | a <- (words sentence), b <- (sentenceToSet ctx)]
  where
    ctx = foldl (++) [] $ intersperse " " context
    
    
-- counts colocations for words in a paragraph
paragraphCounts :: [String] -> [CountPair]
paragraphCounts [x] = []
paragraphCounts sentences = helper ("":sentences)
  where
    helper [] = []
    helper [x] = []
    helper (x:y:[]) = sentenceCounts y [x]
    helper (x:y:z:rest) = sentenceCounts y [x,z] ++ helper (y:z:rest)
    
-- like documentCounts, but uses a HashTable in the IO monad
documentCountsHT :: CountTable -> [String] -> IO (CountTable)
documentCountsHT ht [] = return ht
documentCountsHT ht sentences = let sentences' = dropWhile ((== "") . trim) sentences
                                    paragraph  = takeWhile ((/= "") . trim) sentences'
                                in
                                 do ht <- (accumulateCountsHT ht $ paragraphCounts paragraph)
                                    ht `seq` documentCountsHT ht $ drop (length paragraph) sentences'
                                  
hashStringPair :: (String, String) -> Int32
hashStringPair (w,v) = H.hashString $ w ++ v

emptyCountTable :: IO CountTable
emptyCountTable = H.new (==) hashStringPair

-- like accumulateCounts, but uses a HashTable in the IO monad
accumulateCountsHT :: CountTable -> [CountPair] -> IO (CountTable)
accumulateCountsHT ht counts = do ht <- foldM (\ht (w,v,c) ->
                                                do current <- H.lookup ht (w,v)
                                                   case current of
                                                     Nothing -> H.insert ht (w,v) c
                                                     Just d -> H.update ht (w,v) (c+d) >> return ()
                                                   return ht)
                                       ht
                                       counts
                                  return ht

                                       

-- Merges two count pairs, A and B, by either returning the smallest (accoring to natural ordering of words)
-- or by adding counts if the words in both input pairs are the same.
--
-- Returns its result as a 3 element pair:
--     * 1st: Nothing if A has smaller words than B, Just A otherwise
--     * 2nd: Nothing if B has smaller words than A, Just B otherwise
--     * 3rd: A if its words are smaller than B's
--            B if its words are smaller than A's
--            A + B's count if A and B's words are equal    
mergeCountPairs :: CountPair -> CountPair -> (Maybe CountPair, Maybe CountPair, CountPair)
mergeCountPairs l@(w1,w2,c) r@(v1,v2,d) = case compare (w1,w2) (v1,v2)
                                          of
                                            EQ -> (Nothing, Nothing, (w1,w2,c+d))
                                            LT -> (Nothing, Just r, l)
                                            GT -> (Just l, Nothing, r)


-- merges two sorted streams of count pairs (with no duplicates). The result is a sorted stream
-- of unique pairs. Counts of pairs from both streams that have identical words are added.
-- (where sorted = sorted by words).
merge :: Maybe CountPair -> Maybe CountPair -> 
        IO (Maybe CountPair) -> IO (Maybe CountPair) -> (CountPair -> IO ()) -> 
        IO (Maybe CountPair)
merge left right nextLeft nextRight write =
  do l <- getLeft
     r <- getRight
     case (l,r) of
       (Nothing, Nothing) -> return Nothing
       (Just l,  Just r)  -> let (l',r',combined) = mergeCountPairs l r
                             in do write combined
                                   merge l' r' nextLeft nextRight write
       (Just l,  Nothing) -> do write l
                                merge Nothing Nothing nextLeft nextRight write
       (Nothing, Just r)  -> do write r
                                merge Nothing Nothing nextLeft nextRight write

  where
    getLeft = case left of
                Nothing  -> nextLeft
                l        -> return l
                
    getRight = case right of
                 Nothing -> nextRight
                 r       -> return r
                                     

-- merges counts from two files and writes results to a third file, where
-- files can be any valid handles.
mergeFiles :: Handle -> Handle -> Handle -> IO ()
mergeFiles inFile1 inFile2 outFile = do merge Nothing Nothing (readLine inFile1) (readLine inFile2) write
                                        return ()
  where
    readLine handle = do eof <- hIsEOF handle
                         if eof
                           then return Nothing
                           else do line <- hGetLine handle
                                   return $ Just (deserialize line)
                                   
    write = (hPutStrLn outFile) . serialize
    
    
-- seeks forward to the closest paragraph boundary, or EOF, whichever comes first
hSeekToParagraph :: Handle -> IO ()
hSeekToParagraph h = do hReadOne -- we might by chance be positioned
                                   -- at the end of a non-empty line,
                                   -- right before the newline. This
                                   -- situation is indistinguishable
                                   -- from being on an empty line, so
                                   -- we seek forward one line and
                                   -- then check.
                        helper h
  where
    hReadOne = do eof <- hIsEOF h
                  if eof then return () else hSafeGetLine h >> return ()
                  
    helper h = do eof <- hIsEOF h
                  if eof
                    then return ()
                    else do line <- handle (\e -> do hPutStrLn stderr $ "Warning: " ++ show (e :: SomeException)
                                                     hSeek h RelativeSeek 1
                                                     return "ignore me")
                                    (hGetLine h)
                            if null $ trim line
                              then return ()
                              else hSeekToParagraph h
    

-- splits the file into N chunks at paragraph boundaries, and returns
-- pairs: (start, end) of the chunks. The number of the chunks might
-- be less than N if there are not enough paragraphs in the file.
hSplit :: Handle -> Integer -> IO [(Integer,Integer)]
hSplit handle n = do size <- hFileSize handle
                     let chunkSize = (fromInteger size) / (fromInteger n) :: Double
                         -- boundaries without regard for
                         -- paragraphs. We'll move these to paragraph
                         -- boundaries.
                         byteBoundaries = nub $ map (round . (*chunkSize)) [1..fromInteger (n-1)]
                     paragraphBoundaries <- mapM (\start -> do hSeek handle AbsoluteSeek start
                                                               hSeekToParagraph handle
                                                               hTell handle)
                                            byteBoundaries
                     let boundaries = nub $ sort paragraphBoundaries
                         starts = nub $ 0:boundaries
                         ends = nub $ boundaries++[size]
                     return $ zip starts ends
                     
hSafeGetLine :: Handle -> IO (String)
hSafeGetLine h =  handle (\e -> do hPutStrLn stderr $ "Warning: " ++ show (e :: SomeException)
                                   hSeek h RelativeSeek 1
                                   return "")
                  (hGetLine h)

-- Creates a new temporary file. The created file has to be manually deleted.
newTmpFile :: IO (FilePath, Handle)
newTmpFile = do tmpDir <- getTemporaryDirectory
                openTempFile tmpDir "colocations.txt"
                
-- writes counts to a file, one per line
writeCounts :: Handle -> [CountPair] -> IO ()
writeCounts handle = mapM_ $ (hPutStrLn handle) . serialize

-- copies all data from one handle to another, closing both handles upon
-- completion and removing the file associated with the input handle.
-- If the input is Nothing, this function is a no-op.
hCopyAndDelete :: Maybe (FilePath, Handle) -> Handle -> IO ()
hCopyAndDelete Nothing handle = hClose handle
hCopyAndDelete (Just (path, inHandle)) outHandle = do str <- hGetContents inHandle
                                                      hPutStr outHandle str
                                                      hClose inHandle
                                                      hClose outHandle
                                                      removeFile path
                                                      

countColocations :: FilePath -> [(Integer, Integer)] -> IO (FilePath, Handle)
countColocations inFilePath [(start, end)] = do inFile <- openFile inFilePath ReadMode
                                                hPutStrLn stderr $ show (start, end)
                                                hSeek inFile AbsoluteSeek start
                                                lines <- hReadLines inFile end []
                                                ht <- emptyCountTable
                                                documentCountsHT ht lines
                                                lst <- H.toList ht
                                                let counts = sort $ map (\((w,v), c) -> (w,v,c)) lst
                                                (outPath, outHandle) <- newTmpFile
                                                writeCounts outHandle counts
                                                hSeek outHandle AbsoluteSeek 0
                                                return (outPath, outHandle)
  where
    hReadLines handle end sofar = do pos <- hTell handle
                                     case compare pos end of
                                       EQ -> return $ reverse sofar
                                       GT -> return $ reverse (tail sofar) -- we've read too far
                                       LT -> do line <- hSafeGetLine handle
                                                hReadLines handle end $ line:sofar
                                               
                                               
countColocations inFile [] = error "Cannot count colocations with no chunks."
countColocations inFile chunks = do let (chunksLeft, chunksRight) = halve chunks
                                    (fileLeft, handleLeft) <- countColocations inFile chunksLeft
                                    (fileRight, handleRight) <- countColocations inFile chunksRight
                                    (outFile, outHandle) <- newTmpFile
                                    fileLeft `par` fileRight `par` (mergeFiles handleLeft handleRight outHandle)
                                    hClose handleLeft
                                    hClose handleRight
                                    removeFile fileLeft
                                    removeFile fileRight
                                    hSeek outHandle AbsoluteSeek 0
                                    return (outFile, outHandle)
  where
    halve lst = let k = (length lst) `div` 2
                in (take k lst, drop k lst)
                                    
                                    
                   
countInFile :: Integer -> FilePath -> FilePath -> IO ()
countInFile chunkSize inPath outPath =
  do inHandle <- openFile inPath ReadMode
     inSize <- hFileSize inHandle
     let numChunks = inSize `div` chunkSize
     chunks <- hSplit inHandle numChunks
     hPutStrLn stderr $ "Number of chunks: " ++ show (length chunks)                                     
     hPutStrLn stderr $ "Chunks: " ++ show chunks
     (tempOutPath, tempOutHandle) <- countColocations inPath chunks
     hClose tempOutHandle
     renameFile tempOutPath outPath
     hClose inHandle
                                     

data Options = Options { chunkSize :: String
                       , inputPath :: FilePath
                       } deriving Show

main :: IO ()
main = do args <- getArgs
          countInFile (1024*1024) (args !! 0) (args !! 1)
           
          