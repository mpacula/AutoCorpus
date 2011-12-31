/*
    AutoCorpus: automatically extracts clean natural language corpora from
    publicly available datasets.

    merge.h: merging count files by adding counts



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
*/

#ifndef merge_h
#define merge_h

// Reads two sorted files of counts and merges them into an output
// file with the counts added up.
size_t mergeCounts(FILE* src1, FILE* src2, FILE* out);

#endif
