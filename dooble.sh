#!/bin/sh

export AA_ENABLEHIGHDPISCALING=1
export AA_USEHIGHDPIPIXMAPS=1

# Maximum of 50.

export DOOBLE_ADDRESS_WIDGET_HEIGHT_OFFSET=10

# Maximum of 50.

export DOOBLE_TAB_HEIGHT_OFFSET=5
export QT_AUTO_SCREEN_SCALE_FACTOR=1

# Disable https://en.wikipedia.org/wiki/MIT-SHM.

export QT_X11_NO_MITSHM=1

if [ -r ./Dooble ] && [ -x ./Dooble ]
then
    # Be careful with SSL errors!

    exec ./Dooble \
	 --disable-reading-from-canvas \
	 --ignore-certificate-errors \
         "$@"
    exit $?
elif [ -r /opt/dooble/Dooble ] && [ -x /opt/dooble/Dooble ]
then
    export LD_LIBRARY_PATH=/opt/dooble/Lib
    cd /opt/dooble && exec ./Dooble --disable-reading-from-canvas "$@"
    exit $?
else
    exit 1
fi
