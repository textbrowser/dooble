#!/usr/bin/env bash
# Alexis Megas.

if [ ! -e dooble.pro ]
then
    echo "Please issue $0 from the primary directory."
    exit 1
fi

make distclean 1>/dev/null 2>/dev/null

declare -a qmakes=("$HOME/Qt/6.11.1/macos/bin/qmake"
		   "$HOME/Qt/6.8.3/macos/bin/qmake")
qmake=""

for i in "${qmakes[@]}"
do
    qmake="$(echo $i)"

    if [ -x "$qmake" ]
    then
	break
    fi
done

if [ -x "$qmake" ]
then
    echo "Found $qmake."
    $qmake -o Makefile dooble.pro 1>/dev/null 2>/dev/null
else
    echo "Cannot locate qmake. Please install the official Qt."
    exit 1
fi

echo "Building Dooble."
make -j $(sysctl -n hw.ncpu)
make install
echo "Signing Dooble.d/Dooble.app."
codesign --deep --force -s "textbrowser" ./Dooble.d/Dooble.app \
	 1>/dev/null 2>/dev/null
echo "Building the DMG."
make dmg 1>/dev/null 2>/dev/null

if [ ! -r Dooble.dmg ]
then
    echo "Dooble.dmg is not a readable file."
    exit 1
fi

mv Dooble.dmg Dooble-2026.08.20_Universal.dmg
make distclean 1>/dev/null 2>/dev/null
rm -fr ./Dooble.d
