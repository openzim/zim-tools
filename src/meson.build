
if with_xapian_support
  executable('zimsearch', 'zimsearch.cpp',
    dependencies: libzim_dep,
    install: true)
else
  message('Libzim seems to be compiled without xapian support.\nzimsearch will not be compiled.')
endif

# [FIXME] There are some problem with clock_gettime and mingw.
if target_machine.system() != 'windows'
  executable('zimbench', 'zimbench.cpp',
    dependencies: [libzim_dep, rt_dep],
    install: true)
endif

executable('zimdump', 'zimdump.cpp', 'tools.cpp',
  dependencies: [libzim_dep, docopt_dep],
  install: true)

executable('zimdiff', ['zimdiff.cpp', 'tools.cpp'],
  dependencies: libzim_dep,
  install: true)

executable('zimpatch', ['zimpatch.cpp', 'tools.cpp'],
  dependencies: libzim_dep,
  install: true)

executable('zimsplit', 'zimsplit.cpp',
  dependencies: [libzim_dep, docopt_dep],
  install: true)

executable('zimrecreate', ['zimrecreate.cpp', 'tools.cpp'],
  dependencies: libzim_dep,
  install: true)

subdir('zimcheck')

if with_writer
  subdir('zimwriterfs')
endif
