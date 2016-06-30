AUTOMAKE_OPTIONS=subdir-objects
bin_PROGRAMS=zimwriterfs

zimwriterfs_SOURCES= \
        zimwriterfs.cpp \
        tools.cpp \
        article.cpp \
        articlesource.cpp \
        indexer.cpp \
        resourceTools.cpp \
        pathTools.cpp \
        mimetypecounter.cpp

zimwriterfs_CXXFLAGS = $(ICU_CFLAGS)
zimwriterfs_LDFLAGS = $(ICU_LDFLAGS)

if HAVE_XAPIAN
zimwriterfs_CXXFLAGS += $(XAPIAN_CFLAGS)
zimwriterfs_LDFLAGS += $(XAPIAN_LDFLAGS)
zimwriterfs_SOURCES += \
        xapianIndexer.cpp \
        xapian/myhtmlparse.cc \
        xapian/htmlparse.cc
endif
