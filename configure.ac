# autotools initializations
AC_INIT([zimwriterfs], [0.1])

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

# Check if the liblzma is available
AC_CHECK_HEADER([lzma.h],, [AC_MSG_ERROR([[cannot find lzma header]])])
AC_CHECK_LIB([lzma], [lzma_version_string],, [AC_MSG_ERROR([[cannot find lzma]])])

# Check if the libzim is available
AC_CHECK_HEADER([zim/zim.h],, [AC_MSG_ERROR([[cannot find libzim header]])])
AC_CHECK_LIB([zim], [zim_MD5Init],, [AC_MSG_ERROR([[cannot find libzim]])])

# Check if the libmagic is available
AC_CHECK_HEADER([magic.h],, [AC_MSG_ERROR([[cannot find libmagic header]])])
AC_CHECK_LIB([magic], [magic_file],, [AC_MSG_ERROR([[cannot find libmagic]])])

# Check if the libpthread is available
AC_CHECK_HEADER([pthread.h],, [AC_MSG_ERROR([[cannot find libpthread header]])])
AC_CHECK_LIB([pthread], [pthread_exit],, [AC_MSG_ERROR([[cannot find libpthread]])])

# Set current language to C++
AC_LANG(C++)

# Check the existence of stat64 (to handle file >2GB) in the libc
AC_CHECK_FUNCS([stat64])

# cxxflags
CXXFLAGS="-O3 -Igumbo $CXXFLAGS"
CFLAGS="-O3 -std=gnu99 -std=c99"

# liblzma
LIBLZMA_CFLAGS=""
LIBLZMA_LDFLAGS=" -llzma"

# libzim
LIBZIM_CFLAGS=""
LIBZIM_LDFLAGS=" -lzim"

# libz
LIBZ_CFLAGS=""
LIBZ_LDFLAGS=" -lz"

# libmagic
LIBMAGIC_CFLAGS=""
LIBMAGIC_LDFLAGS=" -lmagic"

# libpthread
LIBPTHREAD_CFLAGS=""
LIBPTHREAD_LDFLAGS=" -lpthread"

AC_DEFINE_UNQUOTED(CLUSTER_CACHE_SIZE, 16, [set zim cluster cache size to number of cached chunks])
AC_DEFINE_UNQUOTED(DIRENT_CACHE_SIZE, 512, [set zim dirent cache size to number of cached chunks])
AC_DEFINE_UNQUOTED(LZMA_MEMORY_SIZE, 128, [set lzma uncompress memory size to number of MB])
AC_DEFINE(ENABLE_LZMA, [1], [defined if lzma compression is enabled])

# export variables
AC_SUBST(CXXFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(LIBLZMA_CFLAGS)
AC_SUBST(LIBLZMA_LDFLAGS)
AC_SUBST(LIBZIM_CFLAGS)
AC_SUBST(LIBZIM_LDFLAGS)
AC_SUBST(LIBZ_CFLAGS)
AC_SUBST(LIBZ_LDFLAGS)
AC_SUBST(LIBMAGIC_CFLAGS)
AC_SUBST(LIBMAGIC_LDFLAGS)
AC_SUBST(LIBPTHREAD_CFLAGS)
AC_SUBST(LIBPTHREAD_LDFLAGS)

# Configure the output files
AC_CONFIG_FILES([
  Makefile
])

AC_PROG_INSTALL
AC_OUTPUT