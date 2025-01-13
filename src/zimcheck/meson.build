if compiler.has_header('mustache.hpp')
  extra_include = []
elif compiler.has_header('mustache.hpp', args: '-I/usr/include/kainjow')
  extra_include = ['/usr/include/kainjow']
else
  error('Cannot find header mustache.hpp')
endif

inc = include_directories(extra_include)

zimcheck_deps = [libzim_dep, icu_dep, docopt_dep]

# C++ std::thread is implemented using pthread on Linux by GCC, and on FreeBSD
# for both GCC and LLVM.
if (host_machine.system() == 'linux' and compiler.get_id() == 'gcc') or host_machine.system() == 'freebsd'
  zimcheck_deps += dependency('threads')
endif

executable('zimcheck',
  'main.cpp',
  'zimcheck.cpp',
  'checks.cpp',
  'json_tools.cpp',
  '../tools.cpp',
  '../metadata.cpp',
  include_directories : inc,
  dependencies: zimcheck_deps,
  install: true)
