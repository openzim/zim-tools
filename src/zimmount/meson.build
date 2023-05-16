sources = [
  'zimmount.cpp'
]

libfuse_dep = dependency('fuse3', version : '>=3.1')

deps = [libzim_dep, libfuse_dep]

executable('zimmount',
           sources,
           dependencies : deps,
           install : true)
