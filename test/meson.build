gtest_dep = dependency('gtest', main:true, fallback:['gtest', 'gtest_main_dep'], required:false)

tests = [
    'tools-test',
    'zimwriterfs-zimcreatorfs',
    'zimcheck-test'
]

zimwriter_srcs = [  '../src/zimwriterfs/tools.cpp',
                    '../src/zimwriterfs/zimcreatorfs.cpp',
                    '../src/zimwriterfs/mimetypecounter.cpp',
                    '../src/tools.cpp']

tests_src_map = { 'zimcheck-test' : ['../src/zimcheck/zimcheck.cpp', '../src/zimcheck/checks.cpp', '../src/tools.cpp'],
                  'tools-test' : zimwriter_srcs,
                  'zimwriterfs-zimcreatorfs' : zimwriter_srcs }

if gtest_dep.found() and not meson.is_cross_build()

    foreach test_name : tests

        test_exe = executable(test_name, [test_name+'.cpp'] + tests_src_map[test_name],
                              dependencies : [gtest_dep, libzim_dep, gumbo_dep, magic_dep, zlib_dep],
                              build_rpath : '$ORIGIN')

        test(test_name, test_exe, timeout : 60,
             workdir: meson.current_source_dir())
    endforeach
endif
