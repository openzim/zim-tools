
executable('zimsearch', 'zimsearch.cpp',
  dependencies: libzim_dep,
  install: true)

# [FIXME] There are some problem with clock_gettime and mingw.
if target_machine.system() != 'windows'
  executable('zimbench', 'zimbench.cpp',
    dependencies: [libzim_dep, rt_dep],
    install: true)
endif

executable('zimdump', 'zimdump.cpp',
  dependencies: [libzim_dep, docopt_dep],
  install: true)

executable('zimdiff', 'zimdiff.cpp',
  dependencies: libzim_dep,
  install: true)

executable('zimpatch', 'zimpatch.cpp',
  dependencies: libzim_dep,
  install: true)

executable('zimsplit', 'zimsplit.cpp',
  dependencies: libzim_dep,
  install: true)

executable('zimcheck', 'zimcheck.cpp', 'tools.cpp',
  dependencies: libzim_dep,
  install: true)

executable('zimrecreate', 'zimrecreate.cpp',
  dependencies: libzim_dep,
  install: true)

if with_writer
  subdir('zimwriterfs')
endif
