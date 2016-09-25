AUTOMAKE_OPTIONS=subdir-objects
bin_PROGRAMS=zimwriterfs

zimwriterfs_SOURCES= \
        zimwriterfs.cpp \
        tools.cpp \
        tools.h \
        article.cpp \
        article.h \
        articlesource.cpp \
        articlesource.h \
        indexer.cpp \
        indexer.h \
        resourceTools.cpp \
        resourceTools.h \
        pathTools.cpp \
        pathTools.h \
        mimetypecounter.cpp \
        mimetypecounter.h \
        queue.h

zimwriterfs_CXXFLAGS = $(ICU_CFLAGS)
zimwriterfs_LDFLAGS = $(ICU_LDFLAGS)

if HAVE_XAPIAN
zimwriterfs_CXXFLAGS += $(XAPIAN_CFLAGS)
zimwriterfs_LDFLAGS += $(XAPIAN_LDFLAGS)
zimwriterfs_SOURCES += \
        xapianIndexer.cpp \
        xapianIndexer.h \
        xapian/myhtmlparse.cc \
        xapian/myhtmlparse.h \
        xapian/htmlparse.cc \
        xapian/htmlparse.h \
        xapian/namedentities.h
endif

if DEBUG
CXXFLAGS += -g3 -O0
else
CXXFLAGS += -O3
endif