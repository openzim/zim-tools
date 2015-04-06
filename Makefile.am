AUTOMAKE_OPTIONS=subdir-objects
bin_PROGRAMS=zimwriterfs
zimwriterfs_SOURCES= zimwriterfs.cpp gumbo/utf8.c gumbo/string_buffer.c gumbo/parser.c gumbo/error.c gumbo/string_piece.c gumbo/tag.c gumbo/vector.c gumbo/tokenizer.c gumbo/util.c gumbo/char_ref.c gumbo/attribute.c
zimwriterfs_CXXFLAGS=$(LIBZIM_CFLAGS) $(LIBLZMA_CFLAGS) $(LIBZ_CFLAGS) -O3
zimwriterfs_LDFLAGS=$(LIBZIM_LDFLAGS) $(LIBLZMA_LDFLAGS) $(LIBZ_LDFLAGS) -lpthread -lmagic