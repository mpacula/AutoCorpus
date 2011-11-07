#!/bin/bash

# This script generates HTML documentation from 
# the man pages.

version="1.0"

start_dir=$( pwd )
main_dir="$( dirname "$0" )/.."
cd "$main_dir"
main_dir=$( pwd )

for src in $( ls "$main_dir/man" | grep -v '~' | grep -P "\\.\\d" ); do
    echo "$src"
    rman --filter HTML "$main_dir/man/$src" -r "%s.%s.html" | \
    sed "s/english/English/g; s/<p>is <a href/is <a href/g; s/This is a new paragraph.$/<p\/>\\0/g; \
        s/This is a test of converting MediaWiki markup into plaintext/<p\/>\\0/g" > \
        "$main_dir/man/html/$src.html"

    cat "$main_dir/man/$src" | gzip > "$main_dir/man/$src.gz"
done

cd "$main_dir/man/html"
pages=$( ls *.html )

echo "Uploading..."
echo -e "mkdir autocorpus\ncd autocorpus\nmkdir $version\ncd $version\nmkdir man" | ftp ftp.mpacula.com

for page in $pages; do
    echo $page
    echo -e "cd autocorpus/$version/man\nput $page" | ftp ftp.mpacula.com
done