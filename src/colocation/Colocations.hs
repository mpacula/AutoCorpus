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

module Colocations where

import Data.List

type CountPair = (String, String, Integer)

sentenceToSet :: String -> [String]
sentenceToSet = nub . words

sentenceCounts :: String -> [String] -> [CountPair]
sentenceCounts sentence context =
  accumulateCounts $ sort [(a,b,1) | a <- (words sentence), b <- (sentenceToSet ctx)]
  where
    ctx = foldl (++) [] $ intersperse " " context
  
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
