#!/bin/bash

if [ "x$KIWIX_ROOT" = "x" ];
	then
	echo "You must define envvironment variable KIWIX_ROOT to the root of Kiwix git repository. Exiting."
	exit 1
fi

ZIM_DIR="${KIWIX_ROOT}/src/dependencies/zimlib-1.2"
LIBZIM_DIR="${ZIM_DIR}/build/lib"
LZMA_DIR="${KIWIX_ROOT}/src/dependencies/xz/build"
LIBLZMA_DIR="${LZMA_DIR}/lib"
MAGIC_DIR="/usr/local/Cellar/libmagic/5.22_1"
LIBMAGIC_DIR="${MAGIC_DIR}/lib"
STATIC_LDFLAGS="${LIBZIM_DIR}/libzim.a ${LIBLZMA_DIR}/liblzma.a ${LIBMAGIC_DIR}/libmagic.a -lz"

CC="clang -O3"
CXX="clang++ -O3"
CXXFLAGS="-Igumbo -I${ZIM_DIR}/include -I${MAGIC_DIR}/include/ -I${LZMA_DIR}/include"
CFLAGS="$CXXFLAGS"
LDFLAGS="-L. -lzim -llzma -lmagic -lz"
SHARED_OUTPUT="zimwriterfs-shared"
STATIC_OUTPUT="zimwriterfs-static"

function compile {
	$CXX $CXXFLAGS -c zimwriterfs.cpp
	$CC $CFLAGS -c gumbo/utf8.c
	$CC $CFLAGS -c gumbo/string_buffer.c
	$CC $CFLAGS -c gumbo/parser.c
	$CC $CFLAGS -c gumbo/error.c
	$CC $CFLAGS -c gumbo/string_piece.c
	$CC $CFLAGS -c gumbo/tag.c
	$CC $CFLAGS -c gumbo/vector.c
	$CC $CFLAGS -c gumbo/tokenizer.c
	$CC $CFLAGS -c gumbo/util.c
	$CC $CFLAGS -c gumbo/char_ref.c
	$CC $CFLAGS -c gumbo/attribute.c
}

echo "Compiling zimwriterfs for OSX as static then shared."

# remove object files
echo "Clean-up repository (*.o, zimwriterfs-*, *.dylib)"
rm *.o ${STATIC_OUTPUT} ${SHARED_OUTPUT} *.dylib

# compile source code
echo "Compile source code file objects"
compile

# link statically
echo "Link statically into ${STATIC_OUTPUT}"
$CXX $CXXFLAGS $STATIC_LDFLAGS -o ${STATIC_OUTPUT} *.o

# copy dylib to current folder
echo "Copy dylibs into the current folder"
cp -v ${KIWIX_ROOT}/src/dependencies/zimlib-1.2/build/lib/libzim.dylib .
cp -v ${KIWIX_ROOT}/src/dependencies/xz/build/lib/liblzma.dylib .
cp -v ${LIBMAGIC_DIR}/libmagic.dylib .
chmod 644 ./libmagic.dylib

# link dynamicaly
echo "Link dynamically into ${SHARED_OUTPUT}"
$CXX $CXXFLAGS $LDFLAGS -o zimwriterfs-shared *.o

echo "Fix install name tool on ${SHARED_OUTPUT}"
install_name_tool -change ${LIBZIM_DIR}/libzim.0.dylib libzim.dylib ${SHARED_OUTPUT}
install_name_tool -change ${LIBLZMA_DIR}/liblzma.5.dylib liblzma.dylib ${SHARED_OUTPUT}
install_name_tool -change ${LIBMAGIC_DIR}/libmagic.1.dylib libmagic.dylib ${SHARED_OUTPUT}
otool -L ${SHARED_OUTPUT}

ls -lh ${STATIC_OUTPUT}
ls -lh ${SHARED_OUTPUT}
