{-
  Counts word colocations in text documents.
  (c) Maciej Pacula 2011
-}

{-# LANGUAGE TypeSynonymInstances #-}

module Main where

import Control.Monad
import Data.List
import Data.Char
import Data.Int
import qualified Data.HashTable as H
import System.IO

type CountPair = (String, Int64)
type CountTable = H.HashTable String Int64

-- Formats a count pair in the ouput format.
serialize :: CountPair -> String
serialize (wv,c) = show c ++ "\t" ++ wv

-- Removes space characters from both sides of a string
trim :: String -> String
trim = trimLeft . trimLeft
       where
         trimLeft = reverse . dropWhile isSpace

-- Converts a sentence to a set of words
sentenceToSet :: String -> [String]
sentenceToSet = nub . words

-- Counts colocations for words in a sentence given its context
-- (surrounding sentences)
sentenceCounts :: String -> [String] -> [CountPair]
sentenceCounts sentence context =
  [(a ++ " " ++ b,1) | a <- (words sentence), b <- (sentenceToSet ctx)]
  where
    ctx = foldl' (++) [] $ intersperse " " context
    
    
-- Finds all colocation counts for a single paragraph. The returned
-- counts might not be aggregated, i.e. a given pair of words might
-- appear more than once in the output.
paragraphCounts :: [String] -> [CountPair]
paragraphCounts [_] = []
paragraphCounts sentences = helper ("":sentences)
  where
    helper [] = []
    helper [_] = []
    helper (x:y:[]) = sentenceCounts y [x]
    helper (x:y:z:rest) = sentenceCounts y [x,z] ++ helper (y:z:rest)
    
-- Counts colocation for a document, i.e. a set of paragraphs
-- separated by empty lines. The returned counts are aggregated,
-- i.e. a given pair of words will only appear once with all its
-- counts added up.
documentCounts :: CountTable -> [String] -> IO ()
documentCounts _ [] = return ()
documentCounts ht sentences = let sentences' = dropWhile ((== "") . trim) sentences
                                  paragraph  = takeWhile ((/= "") . trim) sentences'
                              in
                               do aggregateCounts ht $ paragraphCounts paragraph
                                  documentCounts ht $ drop (length paragraph) sentences'

-- Takes a list of counts where a pair of words may appear more than
-- once, and adds up all the counts for each pair. The results are
-- stored in a HashTable.
aggregateCounts :: CountTable -> [CountPair] -> IO ()
aggregateCounts ht counts = do forM_
                                 counts
                                 (\(wv,c) ->
                                   do current <- H.lookup ht wv
                                      case current of
                                        Nothing -> H.insert ht wv c
                                        Just d  -> H.update ht wv (c+d) >> return ())

                                       
--  Creates a new empty hashtable to store colocation counts.
emptyCountTable :: IO CountTable
emptyCountTable = H.new (==) H.hashString


-- Counts colocations in text from stdin and output sorted counts to
-- stdout.
main :: IO ()
main = do inputStr <- hGetContents stdin
          ht <- emptyCountTable
          documentCounts ht $ lines inputStr
          counts <- H.toList ht
          forM_ (sort counts) $ putStrLn . serialize
