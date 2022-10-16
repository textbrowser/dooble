#!/usr/bin/env bash
# Alexis Megas.

if [ ! -x /usr/bin/dpkg-deb ]; then
    echo "Please install dpkg-deb."
    exit
fi

if [ ! -x /usr/bin/fakeroot ]; then
    echo "Please install fakeroot."
    exit 1
fi

if [ ! -e dooble.pro ]; then
    echo "Please issue $0 from the primary directory."
    exit 1
fi

make distclean 2>/dev/null
mkdir -p ./opt/dooble/Data
mkdir -p ./opt/dooble/Documentation
mkdir -p ./opt/dooble/Lib
mkdir -p ./opt/dooble/Translations
chmod -x,g+w ./opt/dooble/Lib/*
qmake -o Makefile dooble.pro && make -j $(nproc)
cp -p ./Documentation/Documents/*.pdf ./opt/dooble/Documentation/.
cp -p ./Documentation/KDE ./opt/dooble/Documentation/.
cp -p ./Documentation/TO-DO ./opt/dooble/Documentation/.
cp -p ./Documentation/dooble.asc ./opt/dooble/Documentation/.
cp -p ./Documentation/dooble.pol ./opt/dooble/Documentation/.
cp -p ./Dooble ./opt/dooble/.
cp -p ./Icons/Logo/dooble.png ./opt/dooble/.
cp -p ./Translations/*.qm ./opt/dooble/Translations/.
cp -pr ./Charts ./opt/dooble/.
cp -pr ./Data/*.txt ./opt/dooble/Data/.
cp -pr ./Data/README ./opt/dooble/Data/.
find ./opt/dooble/plugins -name '*.so' -exec chmod -x {} \;
mkdir -p dooble-debian.d/opt
mkdir -p dooble-debian.d/usr/bin
mkdir -p dooble-debian.d/usr/share/applications
mkdir -p dooble-debian.d/usr/share/pixmaps
cp -p dooble.desktop dooble-debian.d/usr/share/applications/.
cp -pr DEBIAN-NATIVE dooble-debian.d/DEBIAN
cp -r ./opt/dooble dooble-debian.d/opt/.
cp Icons/Logo/dooble.png dooble-debian.d/usr/share/pixmaps/.
cp dooble.sh dooble-debian.d/usr/bin/dooble
fakeroot dpkg-deb --build dooble-debian.d Dooble-2022.10.15_amd64.deb
make distclean
rm -fr ./opt
rm -fr dooble-debian.d
