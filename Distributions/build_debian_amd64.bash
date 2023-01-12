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
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6Charts.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6Core.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6DBus.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6Gui.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6Network.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6OpenGL.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6OpenGLWidgets.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6Positioning.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6PrintSupport.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6Qml.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6QmlModels.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6Quick.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6QuickWidgets.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6Sql.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6Svg.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6WebChannel.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6WebEngineCore.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6WebEngineWidgets.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6Widgets.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libQt6XcbQpa.so.6 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libicudata.so.56 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libicui18n.so.56 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/lib/libicuuc.so.56 ./opt/dooble/Lib/.
cp -p ~/Qt/6.2.4/gcc_64/libexec/QtWebEngineProcess ./opt/dooble/.
cp -pr ~/Qt/6.2.4/gcc_64/plugins ./opt/dooble/.
cp -pr ~/Qt/6.2.4/gcc_64/resources/* ./opt/dooble/.
cp -pr ~/Qt/6.2.4/gcc_64/translations/*.qm ./opt/dooble/Translations/.
cp -pr ~/Qt/6.2.4/gcc_64/translations/qtwebengine_locales ./opt/dooble/.
chmod -x,g+w ./opt/dooble/Lib/*
~/Qt/6.2.4/gcc_64/bin/qmake -o Makefile dooble.pro && make -j $(nproc)
cp -p ./Documentation/Documents/*.pdf ./opt/dooble/Documentation/.
cp -p ./Documentation/KDE ./opt/dooble/Documentation/.
cp -p ./Documentation/TO-DO ./opt/dooble/Documentation/.
cp -p ./Dooble ./opt/dooble/.
cp -p ./Icons/Logo/dooble.png ./opt/dooble/.
cp -p ./Qt/qt.conf ./opt/dooble/.
cp -p ./Translations/*.qm ./opt/dooble/Translations/.
cp -pr ./Charts ./opt/dooble/.
cp -pr ./Data/*.txt ./opt/dooble/Data/.
cp -pr ./Data/README ./opt/dooble/Data/.
cp -pr ./qtwebengine_dictionaries ./opt/dooble/.
find ./opt/dooble/plugins -name '*.so' -exec chmod -x {} \;
mkdir -p dooble-debian.d/opt
mkdir -p dooble-debian.d/usr/bin
mkdir -p dooble-debian.d/usr/share/applications
mkdir -p dooble-debian.d/usr/share/pixmaps
cp -p dooble.desktop dooble-debian.d/usr/share/applications/.
cp -pr DEBIAN dooble-debian.d/.
cp -r ./opt/dooble dooble-debian.d/opt/.
cp Icons/Logo/dooble.png dooble-debian.d/usr/share/pixmaps/.
cp dooble.sh dooble-debian.d/usr/bin/dooble
fakeroot dpkg-deb --build dooble-debian.d Dooble-0000.00.00_amd64.deb
make distclean
rm -fr ./opt
rm -fr dooble-debian.d
