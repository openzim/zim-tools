configure_file(output : 'config.h',
               configuration : conf,
               input : 'config.h.in')

sources = [
  'zimwriterfs.cpp',
  'tools.cpp',
  'article.cpp',
  'zimcreatorfs.cpp',
  'mimetypecounter.cpp'
]

deps = [thread_dep, libzim_dep, zlib_dep, gumbo_dep, magic_dep]

zimwriterfs = executable('zimwriterfs',
                         sources,
                         dependencies : deps,
                         install : true,
                         install_rpath : join_paths(get_option('prefix'), get_option('libdir')))
