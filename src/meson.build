
executable('zimsearch', 'zimSearch.cpp',
  dependencies: libzim_dep,
  install: true)

# [FIXME] There are some problem with clock_gettime and mingw.
if target_machine.system() == 'windows'
  executable('zimbench', 'zimBench.cpp',
    dependencies: [libzim_dep, rt_dep],
    install: true)
endif

executable('zimdump', 'zimDump.cpp',
  dependencies: libzim_dep,
  install: true)

executable('zimdiff', 'zimDiff.cpp',
  dependencies: libzim_dep,
  install: true)

executable('zimpatch', 'zimPatch.cpp',
  dependencies: libzim_dep,
  install: true)

executable('zimsplit', 'zimSplit.cpp',
  dependencies: libzim_dep,
  install: true)

executable('zimcheck', 'zimCheck.cpp',
  dependencies: libzim_dep,
  install: true)

executable('zimrecreate', 'zimRecreate.cpp',
  dependencies: libzim_dep,
  install: true)
