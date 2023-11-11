#!/usr/bin/env bash
# Alexis Megas.

VERSION=$1

if [ -z "$VERSION" ]; then
    echo "Please specify the version: $0 <VERSION>."
    exit 1
fi

for file in Distributions/*/control; do
    sed -i "s/Version: .*/Version: $VERSION/" $file
done

for file in Distributions/build*; do
    sed -i "s/Dooble-.*_/Dooble-$VERSION\_/" $file
done

FILE="Source/dooble_version.h"

sed -i 's/\(DOOBLE_VERSION_STRING "\)[0-9]\+\(\.[0-9]\+\)*"/\1'"$VERSION"'"/' \
    $FILE

echo "Please modify ReleaseNotes.html."
