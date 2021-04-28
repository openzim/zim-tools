if compiler.has_header('mustache.hpp')
  extra_include = []
elif compiler.has_header('mustache.hpp', args: '-I/usr/include/kainjow')
  extra_include = ['/usr/include/kainjow']
else
  error('Cannot find header mustache.hpp')
endif

inc = include_directories(extra_include)

executable('zimcheck',
  'main.cpp',
  'zimcheck.cpp',
  'checks.cpp',
  'json_tools.cpp',
  '../tools.cpp',
  include_directories : inc,
  dependencies: libzim_dep,
  install: true)


