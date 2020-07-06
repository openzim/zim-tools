
sources = [
  'zimwriterfs.cpp',
  'tools.cpp',
  '../tools.cpp',
  'article.cpp',
  'zimcreatorfs.cpp',
  'mimetypecounter.cpp'
]

deps = [thread_dep, libzim_dep, zlib_dep, gumbo_dep, magic_dep]

zimwriterfs = executable('zimwriterfs',
                         sources,
                         dependencies : deps,
                         install : true)
