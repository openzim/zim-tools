gtest_dep = dependency('gtest', main:true, fallback:['gtest', 'gtest_main_dep'], required:false)


test_deps = [gtest_dep, libzim_dep, icu_dep]
tests = [
    'metadata-test',
    'zimcheck-test'
]

if with_writer
  tests += [
      'tools-test',
      'zimwriterfs-zimcreatorfs'
  ]
  test_deps += [gumbo_dep, magic_dep, zlib_dep]
endif

zimwriter_srcs = [  '../src/zimwriterfs/tools.cpp',
                    '../src/zimwriterfs/zimcreatorfs.cpp',
                    '../src/tools.cpp']

tests_src_map = { 'zimcheck-test' : ['../src/zimcheck/zimcheck.cpp', '../src/zimcheck/checks.cpp',  '../src/zimcheck/json_tools.cpp', '../src/tools.cpp', '../src/metadata.cpp'],
                  'tools-test' : zimwriter_srcs,
                  'metadata-test' : ['../src/metadata.cpp'],
                  'zimwriterfs-zimcreatorfs' : zimwriter_srcs }

if gtest_dep.found() and not meson.is_cross_build()

    foreach test_name : tests

        test_exe = executable(test_name, [test_name+'.cpp'] + tests_src_map[test_name],
                              dependencies : test_deps,
                              include_directories: inc,
                              build_rpath : '$ORIGIN')

        test(test_name, test_exe, timeout : 60,
             workdir: meson.current_source_dir())
    endforeach
endif
