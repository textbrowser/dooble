#!/bin/sh
# Written by publicsite.

if [ ! -f "$1" ]; then
    echo "Arg1: bookmarks.html file"
fi

if [ "$DOOBLE_HOME" = "" ]; then
    DOOBLE_HOME_BOOKMARKS="${HOME}/.dooble"
else
    DOOBLE_HOME_BOOKMARKS="${DOOBLE_HOME}"
fi

if [ -f "${DOOBLE_HOME_BOOKMARKS}/dooble_favicons.db" ]; then
    if [ ! -f "${DOOBLE_HOME_BOOKMARKS}/dooble_favicons.db.bak" ]; then
	cp -a "${DOOBLE_HOME_BOOKMARKS}/dooble_favicons.db" "${DOOBLE_HOME_BOOKMARKS}/dooble_favicons.db.bak"
    fi

    if [ ! -f "${DOOBLE_HOME_BOOKMARKS}/dooble_history.db.bak" ]; then
	cp -a "${DOOBLE_HOME_BOOKMARKS}/dooble_history.db" "${DOOBLE_HOME_BOOKMARKS}/dooble_history.db.bak"
    fi

    grep -o "A HREF.*>" "$1" | while read line; do
	link="$(printf "%s" $line | cut -d ">" -f 1 | cut -d "\"" -f 2)"
	name="$(printf "%s" $line | cut -d ">" -f 2 | cut -d "<" -f 1)"

	if [ "$link" != "" ]; then
	    if [ "$name" = "" ]; then
		name="$link"
	    fi

	    echo "Processing $link"
	    sqlite3 "${DOOBLE_HOME_BOOKMARKS}/dooble_history.db" "INSERT INTO dooble_history(favorite_digest,last_visited,number_of_visits,title,url,url_digest) VALUES (\"$(printf "true" | base64)\",\"$(printf "2000-00-00T00:00:01" | base64)\",\"$(printf "1" | base64)\",\"$(printf "%s" "${name}" | base64)\",\"$(printf "%s" "${link}" | base64)\",\"$(printf "%s" "${link}" | base64)\")"
	fi
    done
else
    echo "Could not find DOOBLE_HOME, exiting!"
fi
