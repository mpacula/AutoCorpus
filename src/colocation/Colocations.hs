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

module Colocations where

import Data.List
import Data.Char
import qualified Data.Map as M
import System.Directory
import System.IO
import Test.QuickCheck

type CountPair = (String, String, Integer)


{-- TESTS --}

instance Arbitrary CountPair where
  arbitrary = do let words =  ["a", "b", "c", "d", "foo", "bar"]
                 w <- elements words
                 v <- elements words
                 c <- choose (1, 10)
                 return (w, v, c)
                  
                   
prop_trim_indempotent str = trim str == (trim . trim) str
prop_trim_nongrow str = length str >= (length . trim) str


prop_sentence_to_set_uniq sentence = (nub (sentenceToSet sentence)) == (sentenceToSet sentence)
prop_sentence_to_set_words sentence = sort (nub (words sentence)) == sort (sentenceToSet sentence)


prop_counts_sentence sentence context = undefined

-- runs all tests
test :: IO ()
test = mapM_ (\(name, props) -> putStrLn name >> mapM_ quickCheck props)
           [ ("trim",          [prop_trim_indempotent, prop_trim_nongrow])
           , ("sentenceToSet", [prop_sentence_to_set_uniq])
           ]


{-- IMPLEMENTATION --}

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
  accumulateCounts $ sort [(a,b,1) | a <- (words sentence), b <- (sentenceToSet ctx)]
  where
    ctx = foldl (++) [] $ intersperse " " context
    
    
-- counts colocations for words in a paragraph
paragraphCounts :: [String] -> [CountPair]
paragraphCounts sentences = accumulateCounts $ sort $ helper ("":sentences)
  where
    helper [] = []
    helper [x] = []
    helper (x:y:[]) = sentenceCounts y [x]
    helper (x:y:z:rest) = sentenceCounts y [x,z] ++ helper (y:z:rest)
    
    
-- counts colocations for a document. Paragraphs are assumed to be separated
-- by one or more blank lines
documentCounts :: [String] -> [CountPair]
documentCounts [] = []
documentCounts sentences = let sentences' = dropWhile ((== "") . trim) sentences
                               paragraph  = takeWhile ((/= "") . trim) sentences'
                               counts = paragraphCounts paragraph ++ documentCounts (drop (length paragraph) sentences')
                           in
                            accumulateCounts $ sort counts
                                  

-- given a sorted list of pairs, adds up the counts for pairs
-- with the same words
accumulateCounts :: [CountPair] -> [CountPair]
accumulateCounts [] = []
accumulateCounts [x] = [x]
accumulateCounts (x:y:xs) = let (w1,w2,c) = x
                                (v1,v2,d) = y
                            in if (w1,w2) == (v1,v2)
                               then (w1,w2,c+d):(accumulateCounts xs)
                               else x:y:(accumulateCounts xs)

                      
-- merges a sorted, unique list of count pairs with count pairs in a file.
-- The results are written out to a user-specified file handle.
mergeWithFile :: Handle -> [CountPair] -> Handle -> IO ()
mergeWithFile inHandle [] outHandle = do str <- hGetContents inHandle
                                         hPutStr outHandle str
mergeWithFile inHandle pairs outHandle = helper inLine pairs
  where
    readLine Nothing = -- todo read line only if the previous line has been consumed. Current code always reads the next one.
    helper = do eof <- hIsEOF inHandle
             if eof
                       then writeCounts outHandle pairs
                       else do line <- hGetLine inHandle
                                       let (w1, w2, c) = deserialize line
                                                          (v1, v2, d) = head pairs
                                                      case compare (w1, w2) (v1, v2) of
                                                        EQ -> do hPutStrLn outHandle $ serialize (w1, w2, c+d)
                                                                 mergeWithFile inHandle (tail pairs) outHandle
                                                        LT -> do hPutStrLn outHandle line
                                                                 mergeWithFile inHandle pairs outHandle
                                                        GT -> do hPutStrLn outHandle $ serialize (head pairs)
                                                                 mergeWithFile inHandle (tail pairs) outHandle
                                                        

-- serializes a count pair into a string in a human-readable format
serialize :: CountPair -> String
serialize (w,v,c) = w ++ " " ++ v ++ "\t" ++ show c

-- deserializes a count pair from a string
deserialize :: String -> CountPair
deserialize str = let (w, rest) = break (== ' ') str
                      (v, rest') = break (== '\t') rest
                      c = read rest'
                  in (trim w, trim v, c)


-- reads full paragraphs from a file so that the total number of lines
-- read is approximately N. In practice the exact number will be N +
-- m, where m is the number of lines remaining to complete paragraph
-- at line N, or <= N if EOF is reached. An empty returned list can be
-- used to check for EOF.
hGetNLineParagraphs :: Integer -> Handle -> IO [String]
hGetNLineParagraphs n handle = do lines <- helper 0 handle []
                                  return $ reverse lines
  where
    helper c handle sofar = do eof <- hIsEOF handle
                               if eof
                                 then return sofar
                                 else do line <- hGetLine handle
                                         if trim line == "" && c >= n
                                           then return $ line:sofar
                                           else helper (c+1) handle (line:sofar)
                                         
                                         
-- options that drive our counting process
data Options = Options {
  maxMemoryLines :: Integer -- maximum number of lines we will hold in memory
}

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
                                                      

-- -- counts all colocations in the given file and writes them out to another file
-- countColocations :: Options -> Handle -> Handle -> IO ()
-- countColocations opts inFile outFile = helper Nothing
--   where
--     helper tmpFile = do lines <- hGetNLineParagraphs (maxMemoryLines opts) inFile
--                         if null lines 
--                           then do 
--                                   return ()
                               
--                          else do let counts = documentCounts lines
--                                  case chunks of
--                                    [] -> return ()