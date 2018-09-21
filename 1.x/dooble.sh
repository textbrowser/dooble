#!/bin/sh

if [ -r /usr/local/dooble/Dooble ] && [ -x /usr/local/dooble/Dooble ]
then
    export LD_LIBRARY_PATH=/usr/local/dooble/Lib
    # Disable https://en.wikipedia.org/wiki/MIT-SHM.
    export QT_X11_NO_MITSHM=1
    cd /usr/local/dooble && exec ./Dooble -style fusion "$@"
    exit $?
else
    exit 1
fi
