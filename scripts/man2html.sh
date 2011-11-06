#!/bin/bash

# This script generates HTML documentation from 
# the man pages.

start_dir=$( pwd )
main_dir="$( dirname "$0" )/.."
cd "$main_dir"
main_dir=$( pwd )

for src in $( ls "$main_dir/man" | grep -v '~' | grep -P "\\.\\d" ); do
    echo "$src"
    rman --filter HTML "$main_dir/man/$src" | \
    sed "s/english/English/g; s/<p>is <a href/is <a href/g; s/This is a new paragraph.$/<p\/>\\0/g; \
        s/This is a test of converting MediaWiki markup into plaintext/<p\/>\\0/g" > \
        "$main_dir/man/html/$src"
done
