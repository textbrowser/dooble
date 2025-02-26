#!/bin/sh
# Written by publicsite.

if [ "$DOOBLE_HOME" = "" ]
then
    DOOBLE_HOME_BOOKMARKS="${HOME}/.dooble"
else
    DOOBLE_HOME_BOOKMARKS="${DOOBLE_HOME}"
fi

if [ -f "${DOOBLE_HOME_BOOKMARKS}/dooble_favicons.db" ]
then
    echo '<!DOCTYPE NETSCAPE-Bookmark-file-1>'
    echo '<!-- This is an automatically generated file.'
    echo '     It will be read and overwritten.'
    echo '     DO NOT EDIT! -->'
    echo '<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=UTF-8">'
    echo '<TITLE>Bookmarks</TITLE>'
    echo '<H1>Bookmarks</H1>'
    printf "<DL><p>\n"
    printf "    <DT><H3 PERSONAL_TOOLBAR_FOLDER=\"true\">From Dooble</H3>\n"
    printf "    <DL><p>\n"
    sqlite3 "${DOOBLE_HOME_BOOKMARKS}/dooble_history.db" \
	    "SELECT * FROM dooble_history" | while read line
    do
	printf "        <DT><A HREF=\"%s\">%s</A>\n" \
	       "$(echo "$line" | cut -d "|" -f 5 | base64 -d -)" \
	       "$(printf "%s" "$line" | cut -d "|" -f 4 | base64 -d -)"
    done
    printf "    </DL><p>\n"
    printf "</DL><p>\n"
else
    echo "Could not find DOOBLE_HOME, exiting!"
fi
