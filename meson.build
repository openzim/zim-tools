project('zim-tools', ['c', 'cpp'],
  version : '3.2.0',
  license : 'GPLv3+',
  default_options : ['c_std=c11', 'cpp_std=c++11', 'werror=true'])

add_global_arguments(['-Werror', '-Wall'], language:'cpp')

add_global_arguments('-DVERSION="@0@"'.format(meson.project_version()), language : 'cpp')

static_linkage = get_option('static-linkage')
compiler = meson.get_compiler('cpp')
if static_linkage
  # Static build is not supported on MacOS
  if host_machine.system() != 'darwin'
     add_global_link_arguments('-static-libstdc++', '--static', language:'cpp')
  endif
  if compiler.get_id() == 'gcc' and build_machine.system() == 'linux'
    add_global_link_arguments('-Wl,--whole-archive', '-lpthread', '-Wl,--no-whole-archive', language:'cpp')
  endif
endif

libzim_dep = dependency('libzim', version : '>=8.0.0', static:static_linkage)
with_xapian_support = compiler.has_header_symbol('zim/zim.h', 'LIBZIM_WITH_XAPIAN')

find_library_in_compiler = meson.version().version_compare('>=0.31.0')
rt_dep = dependency('rt', required:false)
docopt_dep = dependency('docopt', static:static_linkage)

with_writer = host_machine.system() != 'windows'

if with_writer
  thread_dep = dependency('threads')
  zlib_dep = dependency('zlib', static:static_linkage)
  gumbo_dep = dependency('gumbo', static:static_linkage)
  magic_dep = dependency('libmagic', static:static_linkage)
endif

subdir('src')
subdir('test')
