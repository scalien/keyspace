#!/bin/sh

VERSION_MAJOR=`sed -n 's/.*VERSION_MAJOR[[:space:]]*\"\([[:digit:]]*\)\"/\1/p' $1`
VERSION_MINOR=`sed -n 's/.*VERSION_MINOR[[:space:]]*\"\([[:digit:]]*\)\"/\1/p' $1`
VERSION_RELEASE=`sed -n 's/.*VERSION_RELEASE[[:space:]]*\"\([[:digit:]]*\)\"/\1/p' $1`

echo $VERSION_MAJOR.$VERSION_MINOR.$VERSION_RELEASE

