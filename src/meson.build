configure_file(output : 'config.h',
               configuration : conf,
               input : 'config.h.in')

common_sources = [
  'zimwriterfs.cpp',
  'tools.cpp',
  'article.cpp',
  'articlesource.cpp',
  'indexer.cpp',
  'resourceTools.cpp',
  'pathTools.cpp',
  'mimetypecounter.cpp'
]

xapian_sources = [
  'xapianIndexer.cpp',
  'xapian/myhtmlparse.cc',
  'xapian/htmlparse.cc'
]

sources = common_sources
deps = [thread_dep, libzim_dep, zlib_dep, gumbo_dep, magic_dep]

if xapian_dep.found()
  sources += xapian_sources
  deps += [xapian_dep, icu_dep]
endif    

zimwriterfs = executable('zimwriterfs',
                         sources,
                         dependencies : deps,
                         install : true,
                         install_rpath : join_paths(get_option('prefix'), get_option('libdir')))
