#!/bin/bash

start_dir=$( pwd )
main_dir="$( dirname "$0" )/.."
cd "$main_dir"
main_dir=$( pwd )
cd $main_dir

. scripts/read-netrc.sh

ver=$( cat VERSION | grep -P -o "\d.\d+.*$" )

echo "Uploading documentation..."
cd "$main_dir"
scripts/generate-doc.sh --upload

echo "Uploading binaries..."
cd "$main_dir/releases/$ver/"
for f in $( ls *.tar.gz ); do
    ftpup "autocorpus/releases/$ver/" "$f"
done

cd "$main_dir/releases/$ver/deb"
for f in $( ls *.deb ); do
    ftpup "autocorpus/releases/$ver/debian" "$f"
done
