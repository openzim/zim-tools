#!/usr/bin/bash

files=(
"article.cpp"
"article.h"
"articlesource.cpp"
"articlesource.h"
"indexer.cpp"
"indexer.h"
"mimetypecounter.cpp"
"mimetypecounter.h"
"pathTools.cpp"
"pathTools.h"
"resourceTools.cpp"
"resourceTools.h"
"tools.cpp"
"tools.h"
"xapianIndexer.cpp"
"xapianIndexer.h"
"zimwriterfs.cpp"
"xapian/htmlparse.cc"
"xapian/htmlparse.h"
"xapian/myhtmlparse.cc"
"xapian/myhtmlparse.h"
"xapian/namedentities.h"
)

for i in "${files[@]}"
do
  echo $i
  clang-format -i -style=file $i
done
