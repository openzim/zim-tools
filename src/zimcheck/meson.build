if compiler.has_header('mustache.hpp')
  extra_include = []
elif compiler.has_header('mustache.hpp', args: '-I/usr/include/kainjow')
  extra_include = ['/usr/include/kainjow']
else
  error('Cannot find header mustache.hpp')
endif

inc = include_directories(extra_include)

if compiler.get_id() == 'gcc' and host_machine.system() == 'linux'
  # C++ std::thread is implemented using pthread on linux by gcc
  thread_dep = dependency('threads')
else
  thread_dep = dependency('', required:false)
endif

executable('zimcheck',
  'main.cpp',
  'zimcheck.cpp',
  'checks.cpp',
  'json_tools.cpp',
  '../tools.cpp',
  '../metadata.cpp',
  include_directories : inc,
  dependencies: [libzim_dep, icu_dep, thread_dep],
  install: true)


