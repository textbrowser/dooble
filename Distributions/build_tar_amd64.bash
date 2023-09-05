#!/usr/bin/env bash
# Alexis Megas.

if [ ! -r dooble.pro ]; then
    echo "Please execute $0 from the primary directory."
    exit 1
fi

# Prepare ./dooble.

make distclean 2>/dev/null
mkdir -p ./dooble/Data
mkdir -p ./dooble/Documentation
mkdir -p ./dooble/Lib
mkdir -p ./dooble/Translations
mkdir -p ./dooble/translations
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6Charts.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6Core.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6DBus.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6Gui.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6Network.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6OpenGL.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6OpenGLWidgets.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6Positioning.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6PrintSupport.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6Qml.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6QmlModels.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6Quick.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6QuickWidgets.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6Sql.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6Svg.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6WebChannel.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6WebEngineCore.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6WebEngineWidgets.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6Widgets.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libQt6XcbQpa.so.6 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libicudata.so.56 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libicui18n.so.56 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/lib/libicuuc.so.56 ./dooble/Lib/.
cp -p ~/Qt/6.5.2/gcc_64/libexec/QtWebEngineProcess ./dooble/.
cp -pr ~/Qt/6.5.2/gcc_64/plugins ./dooble/.
cp -pr ~/Qt/6.5.2/gcc_64/resources ./dooble/.
cp -pr ~/Qt/6.5.2/gcc_64/translations/*.qm ./dooble/Translations/.
cp -pr ~/Qt/6.5.2/gcc_64/translations/qtwebengine_locales \
   ./dooble/translations/.
cp /usr/lib/x86_64-linux-gnu/nss/libsoftokn3.so ./dooble/Lib/.
chmod -x,g+rw ./dooble/Lib/*
~/Qt/6.5.2/gcc_64/bin/qmake -o Makefile dooble.pro && make -j $(nproc)
cp -p ./Documentation/Documents/*.pdf ./dooble/Documentation/.
cp -p ./Documentation/KDE ./dooble/Documentation/.
cp -p ./Documentation/TO-DO ./dooble/Documentation/.
cp -p ./Dooble ./dooble/.
cp -p ./Icons/Logo/dooble.png ./dooble/.
cp -p ./Qt/qt.conf ./dooble/.
cp -p ./Translations/*.qm ./dooble/Translations/.
cp -p ./dooble.sh ./dooble/.
cp -pr ./Charts ./dooble/.
cp -pr ./Data/*.txt ./dooble/Data/.
cp -pr ./Data/README ./dooble/Data/.
cp -pr ./qtwebengine_dictionaries ./dooble/.
find ./dooble/plugins -name '*.so' -exec chmod -x {} \;

# Prepare a tar bundle.

cp $(ldd ./Dooble | awk '{print $3}' | grep -e '^/') ./dooble/Lib/.
chmod -x,g+rw ./dooble/Lib/*
tar -cvz -f Dooble-2023.08.30.tar.gz ./dooble
make distclean
rm -fr ./dooble
