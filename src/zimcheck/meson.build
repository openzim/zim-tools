if compiler.has_header('mustache.hpp')
  extra_include = []
elif compiler.has_header('mustache.hpp', args: '-I/usr/include/kainjow')
  extra_include = ['/usr/include/kainjow']
else
  error('Cannot find header mustache.hpp')
endif

executable('zimcheck',
  'main.cpp',
  'zimcheck.cpp',
  'checks.cpp',
  '../tools.cpp',
  dependencies: libzim_dep,
  install: true)


