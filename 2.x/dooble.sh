#!/bin/sh

export AA_ENABLEHIGHDPISCALING=1
export AA_USEHIGHDPIPIXMAPS=1
export LD_LIBRARY_PATH=/usr/local/dooble/Lib

# Disable https://en.wikipedia.org/wiki/MIT-SHM.

export QT_X11_NO_MITSHM=1

if [ -r ./Dooble ] && [ -x ./Dooble ]
then
    exec ./Dooble -style fusion "$@"
    exit 1
elif [ -r /usr/local/dooble/Dooble ] && [ -x /usr/local/dooble/Dooble ]
then
    cd /usr/local/dooble && exec ./Dooble -style fusion "$@"
    exit $?
else
    exit 1
fi
