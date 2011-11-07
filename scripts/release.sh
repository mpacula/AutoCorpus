#!/bin/bash

# The main release scripts: generates tarballs, deb packages, documentaton etc.
# Uploads everything to the server.

start_dir=$( pwd )
main_dir="$( dirname "$0" )/.."
cd "$main_dir"
main_dir=$( pwd )
cd $main_dir

ver=$( cat VERSION | grep -P -o "\d.\d+.*$" )

echo "Releasing Autocorpus $ver"

rm -r "releases/$ver"
mkdir -p "releases/$ver"
cd "releases/$ver"

echo "Generating source tarball..."
src_name="autocorpus-$ver"
mkdir -p "$src_name/bin"
cd "$src_name"

# all files we'll put in the tarball. Dirty, I know...
files=$( find "$main_dir" -type f | grep -v "/\\.\w" | grep -v "~" | grep -v "/data" | \
    grep -v "/scripts" | grep -v "\\.o" | grep -v "\\.pyc" | grep -v "TAGS" | grep -v -P "/man/.*.\d$" |
    grep -v "/releases" | grep -v "/bin" | sort )

for f in $files; do
    rel_name=${f/"$main_dir"/}
    rel_name=${rel_name/#\//}
    echo -e "\t$rel_name"

    mkdir -p $( dirname "$rel_name" )
    cp "$f" "$rel_name"    
done

cd ..

tar -czf "$src_name.tar.gz" "$src_name"
rm -r "$src_name"


cd "$main_dir/releases/$ver"
echo "Building binary tarball"
arch=$( uname -m )
echo -e "\tarchitecture: $arch"

tar xvf "$src_name.tar.gz" > /dev/null
cd "$src_name/src"
make > /dev/null
result=$?

if [ $result -ne 0 ]; then
    echo "make invocation failed with code $result"
    exit 1
else
    echo "Build successful"
fi 

cd ..
# remove source folder
rm -r src
cd ..

dist="autocorpus-$ver"
bin_name="autocorpus-$ver-$arch"
tar -czf "$bin_name.tar.gz" "$dist"

echo "Binary tarball generated successfully"
rm -r "$dist"
