#!/bin/bash

# This script generates HTML documentation from 
# the man pages.

start_dir=$( pwd )
main_dir="$( dirname "$0" )/.."
cd "$main_dir"
main_dir=$( pwd )

version=$( cat VERSION | grep -P -o "\d.\d+.*$" )

rm -f "$main_dir/man/html/*"

for src in $( ls "$main_dir/man" | grep -v '~' | grep -P "\\.\\d" | grep -v "\.gz" ); do
    echo "$src"
    rman --filter HTML "$main_dir/man/$src" -r "%s.%s.html" | \
    sed "s/english/English/g; s/<p>is <a href/is <a href/g; s/This is a new paragraph.$/<p\/>\\0/g; \
        s/This is a test of converting MediaWiki markup into plaintext/<p\/>\\0/g" > \
        "$main_dir/man/html/$src.html"

    cat "$main_dir/man/$src" | gzip > "$main_dir/man/$src.gz"
done

cd "$main_dir/man/html"
pages=$( ls *.html )

if [[ "$1" == "--upload" ]]; then
    echo "Uploading..."
    echo -e "prompt\nmkdir autocorpus\ncd autocorpus\nmkdir releases\ncd releases\nmkdir $version\ncd $version\n \
             mkdir man\ncd man\nmdelete *" | ftp ftp.mpacula.com
    
    for page in $pages; do
        echo $page
        echo -e "cd autocorpus/releases/$version/man\nput $page" | ftp ftp.mpacula.com
    done
fi
