#!/bin/sh

if [ -r /usr/local/dooble/Dooble ] && [ -x /usr/local/dooble/Dooble ]
then
    export LD_LIBRARY_PATH=/usr/local/dooble/Lib
    cd /usr/local/dooble && exec ./Dooble -style fusion "$@"
    exit $?
else
    exit 1
fi
