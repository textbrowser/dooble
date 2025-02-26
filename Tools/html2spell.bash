#!/usr/bin/env bash

# Alexis Megas.

sed 's/<b>//' $1 | sed 's/<\/b>//' | sed 's/<li>//' | sed 's/<\/li>//' | spell
