# autotools initializations
AC_INIT([zimwriterfs], [1.0])

# where are autotools scripts located
AC_CONFIG_AUX_DIR([.])

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

# find CC compiler
AC_PROG_CC

# find C++ compiler
AC_PROG_CXX
AC_CHECK_TOOL(HAVE_CXX, "$CXX")
if test [ ! "$HAVE_CXX" ]
then
    AC_MSG_ERROR([[cannot find any C++ compiler]])
fi

# Set current language to C++
AC_LANG(C++)

# Determine libtool
AC_PROG_LIBTOOL
AC_PROG_RANLIB
AC_PROG_MAKE_SET

# Check if the 'pkg-config' binary exists
AC_CHECK_TOOL(HAVE_PKG_CONFIG, pkg-config)
if test [ ! "$HAVE_PKG_CONFIG" ]
then
  AC_MSG_ERROR([[cannot find pkg-config]])
fi

# Set up CXXFLAGS/LDFLAGS and ensure they are substituted
AC_ARG_VAR(CXXFLAGS, [C++ compiler flags])
AC_ARG_VAR(LDFLAGS, linker flags)
CXXFLAGS="-O3 $CXXFLAGS"

# Check if the liblzma library is available
AC_CHECK_HEADER([lzma.h],, [AC_MSG_ERROR([[cannot find lzma header]])])
AC_CHECK_LIB([lzma], [lzma_version_string],, [AC_MSG_ERROR([[cannot find lzma]])])

# Check if the libzim library is available
AC_CHECK_HEADER([zim/zim.h],, [AC_MSG_ERROR([[cannot find libzim header]])])
AC_CHECK_LIB([zim], [zim_MD5Init],, [AC_MSG_ERROR([[cannot find libzim]])])

# Check if the libz library is available
AC_CHECK_HEADER([zlib.h],, [AC_MSG_ERROR([[cannot find libz header]])])
AC_CHECK_LIB([z], [deflate],, [AC_MSG_ERROR([[cannot find libz]])])

# Check if the libmagic library is available
AC_CHECK_HEADER([magic.h],, [AC_MSG_ERROR([[cannot find libmagic header]])])
AC_CHECK_LIB([magic], [magic_file],, [AC_MSG_ERROR([[cannot find libmagic]])])

# Check if the libpthread library is available
AC_CHECK_HEADER([pthread.h],, [AC_MSG_ERROR([[cannot find libpthread header]])])
AC_CHECK_LIB([pthread], [pthread_exit],, [AC_MSG_ERROR([[cannot find libpthread]])])

# Check if the gumbo library is available
AC_CHECK_HEADER([gumbo.h],, [AC_MSG_ERROR([[cannot find libgumbo header]])])
AC_CHECK_LIB([gumbo], [gumbo_parse],, [AC_MSG_ERROR([[cannot find libgumbo]])])


# Check the existence of stat64 (to handle file >2GB) in the libc
AC_CHECK_FUNCS([stat64])

AC_DEFINE_UNQUOTED(CLUSTER_CACHE_SIZE, 16, [set zim cluster cache size to number of cached chunks])
AC_DEFINE_UNQUOTED(DIRENT_CACHE_SIZE, 512, [set zim dirent cache size to number of cached chunks])
AC_DEFINE_UNQUOTED(LZMA_MEMORY_SIZE, 128, [set lzma uncompress memory size to number of MB])
AC_DEFINE(ENABLE_LZMA, [1], [defined if lzma compression is enabled])


function findLibrary {
	found=0
	for f in $(echo $LIBS_ROOT|tr ":" "\n") ; do
		sf=`find $f -name $1 | grep $ARCH | head -1 2> /dev/null`
		if [[ -f "$sf" -a $found -eq 0 ]]
		then
			found=1
			echo $sf
		fi
	done
	if [[ $found -eq 0 ]]
	then
		for f in $(echo $LIBS_ROOT|tr ":" "\n") ; do
			sf=`find $f -name $1 | head -1 2> /dev/null`
			if [[ -f "$sf" -a $found -eq 0 ]]
			then
				found=1
				echo $sf
			fi
		done
	fi
	if [[ $found -eq 0 ]]
	then
		echo "no"
	fi
}


####################################################
############ ICU
####################################################


ICU_CFLAGS=""
ICU_LDFLAGS="-licui18n -licuuc -licudata" # replaced by icu-config
ICU_STATIC_LDFLAGS=""

# if --with-x, add path to LIBRARY_PATH
AC_ARG_WITH(icu,
            AC_HELP_STRING([--with-icu=DIR], [alternate location for icu-config]),
            export LIBRARY_PATH="${withval}:${LIBRARY_PATH}";ICU_PATH=${withval}
           )

# look for shared library.
# AC_CHECK_HEADER([zlib.h],, [AC_MSG_ERROR([[cannot find zlib header]])])
# AC_CHECK_LIB([z], [zlibVersion],, [AC_MSG_ERROR([[cannot find zlib]]);COMPILE_ICU=1])
# ICU_FILES=`findLibrary "libicuuc.${SHARED_EXT}"`

AC_CHECK_TOOL(HAVE_ICU_CONFIG, icu-config,, "${ICU_PATH}:${PATH}")
if test [ ! "$HAVE_ICU_CONFIG" ]
then
     AC_MSG_ERROR([[cannot find icu-config]])
else
    OLDPATH=$PATH
    PATH="${ICU_PATH}:${PATH}"
    ICU_CFLAGS=`icu-config --cxxflags`;
    ICU_LDFLAGS=`icu-config --ldflags`;
    ICU_VER=`icu-config --version`;
    ICU_FILES="`findLibrary "libicuuc.${SHARED_EXT}"` `findLibrary "libicudata.${SHARED_EXT}"` `findLibrary "libicui18n.${SHARED_EXT}"`"
    PATH=$OLDPATH
    if [[ $ICU_VER \< "4.2" ]]
	then
	     AC_MSG_ERROR([[You need a version of libicu >= 4.2]])
	fi
fi


AC_SUBST(ICU_CFLAGS)
AC_SUBST(ICU_LDFLAGS)
AC_SUBST(ICU_STATIC_LDFLAGS)
AC_SUBST(ICU_FILES)
AC_SUBST(COMPILED_ICUDATA_DAT)

####################################################
############ XAPIAN
####################################################

XAPIAN_CFLAGS=""
XAPIAN_LDFLAGS=""
XAPIAN_STATIC_LDFLAGS=""
XAPIAN_ENABLE=0

# if --with-x, add path to LIBRARY_PATH
AC_ARG_WITH([xapian],
	[AS_HELP_STRING([--with-xapian=DIR], [alternat location for xapian-config] @@)],
	[xapian_dir=$withval],
	[with_xapian=yes])


AS_IF([test "x$with_xapian" == xno],
        [AM_CONDITIONAL(HAVE_XAPIAN, false)],
	[OLDPATH=$PATH
	 AS_IF([test "x$with_xapian" != xyes],
	       PATH="$with_xapian:$PATH")
	 AC_CHECK_TOOLS(XAPIAN_CONFIG, xapian-config-1.3, xapian-config,[],$PATH)
	 AS_IF([test "x$XAPIAN_CONFIG" == x ],
	        AC_MSG_ERROR([[cannot find xapian-config file]])
	      )
	 XAPIAN_VERSION=`$XAPIAN_CONFIG --version`
	 good_version=yes
	 AS_VERSION_COMPARE($XAPIAN_VERSION, "xapian-config - xapian-core 1.3.4", [good_version=no], [], [])
	 AS_IF([test "x$good_version" == xno],
	        AC_MSG_ERROR([[xapian version must be >= 1.3.4]])
	      )
	 AM_CONDITIONAL(HAVE_XAPIAN, true)
	 AC_DEFINE(HAVE_XAPIAN)
	 XAPIAN_CFLAGS=`$XAPIAN_CONFIG --cxxflags`;
	 XAPIAN_LDFLAGS=`$XAPIAN_CONFIG --ltlibs`;
	 PATH=$OLDPATH
	])

AC_SUBST(XAPIAN_CFLAGS)
AC_SUBST(XAPIAN_LDFLAGS)

# Configure the output files
AC_CONFIG_FILES([
  Makefile
])

AC_PROG_INSTALL
AC_OUTPUT
