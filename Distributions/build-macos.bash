#!/usr/bin/env bash
# Alexis Megas.

if [ ! -e dooble.pro ]
then
    echo "Please issue $0 from the primary directory."
    exit 1
fi

make distclean 2>/dev/null

if [ -x ~/Qt/6.8.3/macos/bin/qmake ]
then
    ~/Qt/6.8.3/macos/bin/qmake
else
    echo "Please install the official Qt."
    exit 1
fi

make -j 5
make install
make dmg

if [ ! -r Dooble.dmg ]
then
    echo "Dooble.dmg is not a readable file."
    exit 1
fi

mv Dooble.dmg Dooble-2025.11.25_Universal.dmg
make distclean 2>/dev/null
