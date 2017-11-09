#!/bin/sh

export LD_LIBRARY_PATH=/usr/local/dooble/Lib

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
