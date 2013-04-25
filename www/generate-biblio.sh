#!/bin/bash

ORIG_BIB_NAME=$1
LAYOUT_BIB_NAME=$2
STYLE=plain

ORIG_DIR_PATH=$(dirname $1)
BASE_PATH="$(echo $ORIG_DIR_PATH | sed -e 's/[^/]*/\.\./g')"

usage() {
		echo "Usage: $0 bib_name layout_name start_date end_date"
		exit 1
}

[ -z "$ORIG_BIB_NAME" ] && usage
[ -z "$LAYOUT_BIB_NAME" ] && usage

mkdir -p tmp-biblio
cd tmp-biblio

one-preambule() {
echo '<?php'
echo 'include("root.php");'
echo 'include($ROOT."common.php");'
echo 'preambule("'$1'", "publi");'
echo '?>'
echo "<p>"
}

one-append() {
		cat $1.html | sed -e 's/<hr><p><em>This file was.*//' \
				| grep -v "bibtex2html" | sed -e 's/<h1>.*<\/h1>//' \
				| sed -e 's/\.html/\.php/' >> $1.php
}

one-epilogue() {
		echo '<?php epilogue() ?>' >> $1
		mv $1 ../$ORIG_DIR_PATH
}

preambule() {
		one-preambule "$1" > "$1".php
		one-preambule "$1 - Abstracts" > "$1"_abstracts.php
		one-preambule "$1 - Bibtex" > "$1"_bib.php
}

append() {
		one-append "$1"
		one-append "$1"_abstracts
		one-append "$1"_bib
}

epilogue() {
		one-epilogue "$1".php $2
		one-epilogue "$1"_abstracts.php $2 _abstracts
		one-epilogue "$1"_bib.php
}

generate() {
		NAME=$3
		cat ../$2 ../$1 > $NAME.bib
		preambule "$NAME"
		TMPDIR=. bibtex2html -q -d -nodoc -noheader -s "$STYLE" -r -t "" -both "$NAME".bib
		append $NAME
		epilogue $NAME $GO
}

generate $ORIG_BIB_NAME $LAYOUT_BIB_NAME Publications

cd ..
rm -Rf tmp-biblio

exit 0