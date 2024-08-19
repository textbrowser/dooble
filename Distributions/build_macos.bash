#!/usr/bin/env bash
# Alexis Megas.

if [ ! -e dooble.pro ]; then
    echo "Please issue $0 from the primary directory."
    exit 1
fi

make distclean 2>/dev/null
qmake6 -o Makefile dooble.pro
make -j 5
make install
make dmg

if [ ! -r Dooble.dmg ]; then
    echo "Dooble.dmg is not a readable file."
    exit 1
fi

if [ "$(uname -m)" = "arm64" ]; then
    mv Dooble.dmg ~/Dooble-2024.08.19_Apple_Silicon.dmg
else
    mv Dooble.dmg ~/Dooble-2024.08.19_Intel.dmg
fi

make distclean 2>/dev/null
