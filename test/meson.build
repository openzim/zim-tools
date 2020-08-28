gtest_dep = dependency('gtest', main:true, fallback:['gtest', 'gtest_main_dep'], required:false)

tests = [
    'tools-test',
    'zimwriterfs-article',
    'zimwriterfs-zimcreatorfs'
]

zimwriter_srcs = [  '../src/zimwriterfs/article.cpp',
                    '../src/zimwriterfs/tools.cpp',
                    '../src/zimwriterfs/zimcreatorfs.cpp',
                    '../src/zimwriterfs/mimetypecounter.cpp',
                    '../src/tools.cpp']

if gtest_dep.found() and not meson.is_cross_build()

    foreach test_name : tests

        test_exe = executable(test_name, [test_name+'.cpp'] + zimwriter_srcs,
                              dependencies : [gtest_dep, libzim_dep, gumbo_dep, magic_dep, zlib_dep],
                              build_rpath : '$ORIGIN')

        test(test_name, test_exe, timeout : 60,
             workdir: meson.current_source_dir())
    endforeach
endif
