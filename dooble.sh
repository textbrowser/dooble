#!/usr/bin/env sh

# Alexis Megas.

dictionaries=qtwebengine_dictionaries

export AA_ENABLEHIGHDPISCALING=1
export AA_USEHIGHDPIPIXMAPS=1

if [ ! -z "$(which qt6ct)" ]
then
    echo "Exporting QT_QPA_PLATFORMTHEME as qt6ct."

    export QT_QPA_PLATFORMTHEME=qt6ct
fi

# Maximum of 50.

export DOOBLE_ADDRESS_WIDGET_HEIGHT_OFFSET=0

# Must be a valid URL! 1024 characters or fewer.

export DOOBLE_GOOGLE_TRANSLATE_URL="https://%1.translate.goog/\
%2?_x_tr_sl=auto&_x_tr_tl=%3&_x_tr_hl=%3&_x_tr_pto=wapp"

# Maximum of 50.

export DOOBLE_TAB_HEIGHT_OFFSET=5

# Automatic scaling.

export QT_AUTO_SCREEN_SCALE_FACTOR=1

# Disable https://en.wikipedia.org/wiki/MIT-SHM.

export QT_X11_NO_MITSHM=1

if [ -r ./Dooble ] && [ -x ./Dooble ]
then
    echo "Launching a local Dooble."

    export DOOBLE_TRANSLATIONS_PATH=Translations
    export QTWEBENGINE_DICTIONARIES_PATH=$dictionaries

    if [ -r ./Lib ]
    then
	export LD_LIBRARY_PATH=Lib
    fi

    if [ -r ./plugins ]
    then
	export QT_PLUGIN_PATH=plugins
    fi

    ./Dooble "$@"
    exit $?
elif [ -r /opt/dooble/Dooble ] && [ -x /opt/dooble/Dooble ]
then
    echo "Launching an official Dooble."

    export DOOBLE_TRANSLATIONS_PATH=/opt/dooble/Translations
    export LD_LIBRARY_PATH=/opt/dooble/Lib
    export QTWEBENGINE_DICTIONARIES_PATH=/opt/dooble/$dictionaries
    export QT_PLUGIN_PATH=/opt/dooble/plugins

    /opt/dooble/Dooble "$@"
    exit $?
elif [ -r /usr/local/dooble/Dooble ] && [ -x /usr/local/dooble/Dooble ]
then
    echo "Launching an official Dooble."

    export DOOBLE_TRANSLATIONS_PATH=/usr/local/dooble/Translations
    export LD_LIBRARY_PATH=/usr/local/dooble/Lib
    export QTWEBENGINE_DICTIONARIES_PATH=/usr/local/dooble/$dictionaries
    export QT_PLUGIN_PATH=/usr/local/dooble/plugins

    /usr/local/dooble/Dooble "$@"
    exit $?
else
    echo "Cannot find Dooble. Please contact your lovely administrator."
    exit 1
fi
