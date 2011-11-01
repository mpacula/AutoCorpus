#!/bin/bash

main_dir="$( dirname "$0" )/.."
cd "$main_dir"
main_dir=$( pwd )

cd "$main_dir"

function prepend() {
    cat "$1" >> "$2.new"
    cat "$2" >> "$2.new"
    mv "$2.new" "$2"
}

function grep_license() {
    count=$( grep -c "Public License" "$1" )
}

for f in $( find src/ -iname "*.cpp" ) $( find src/ -iname "*.h" );
do
    echo -n "$f: "
    grep_license "$f"
    if [ "$count" != "0" ];
    then
        echo "Already has license. Skipping."
    else
        prepend "scripts/COPYING.c.txt" "$f"
        echo "License added successfully."
    fi
done

for f in $( find src/ -iname "*.py" );
do
    echo -n "$f: "
    grep_license "$f"
    if [ "$count" != "0" ];
    then
        echo "Already has license. Skipping."
    else
        prepend "scripts/COPYING.py.txt" "$f"
        echo "License added successfully."
    fi
done
