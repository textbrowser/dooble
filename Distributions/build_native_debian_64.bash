#!/usr/bin/env bash
# Alexis Megas.

if [ ! -x /usr/bin/dpkg-deb ]; then
    echo "Please install dpkg-deb."
    exit 1
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
mkdir -p ./opt/dooble/Translations

if [ -x /usr/bin/qmake6 ]; then
    qmake6 -o Makefile dooble.pro && make -j $(nproc)
else
    qmake -o Makefile dooble.pro && make -j $(nproc)
fi

cp -p ./Documentation/Documents/*.pdf ./opt/dooble/Documentation/.
cp -p ./Documentation/KDE ./opt/dooble/Documentation/.
cp -p ./Documentation/TO-DO ./opt/dooble/Documentation/.
cp -p ./Dooble ./opt/dooble/.
cp -p ./Icons/Logo/dooble.png ./opt/dooble/.
cp -p ./Translations/*.qm ./opt/dooble/Translations/.
cp -pr ./Charts ./opt/dooble/.
cp -pr ./Data/*.txt ./opt/dooble/Data/.
cp -pr ./Data/README ./opt/dooble/Data/.
cp -pr ./qtwebengine_dictionaries ./opt/dooble/.
mkdir -p dooble-debian.d/opt
mkdir -p dooble-debian.d/usr/bin
mkdir -p dooble-debian.d/usr/share/applications
mkdir -p dooble-debian.d/usr/share/pixmaps
cp -p Distributions/dooble.desktop dooble-debian.d/usr/share/applications/.

if [ "$(uname -m)" = "aarch64" ]; then
    cp -pr Distributions/PiOS dooble-debian.d/DEBIAN
else
    cp -pr Distributions/UBUNTU-NATIVE-24.04 dooble-debian.d/DEBIAN
fi

cp -r ./opt/dooble dooble-debian.d/opt/.
cp Icons/Logo/dooble.png dooble-debian.d/usr/share/pixmaps/.
cp dooble.sh dooble-debian.d/usr/bin/dooble

if [ "$(uname -m)" = "aarch64" ]; then
    fakeroot dpkg-deb --build dooble-debian.d Dooble-2024.12.30_arm64.deb
else
    fakeroot dpkg-deb --build dooble-debian.d Dooble-2024.12.30_amd64.deb
fi

make distclean
rm -fr ./opt
rm -fr dooble-debian.d
