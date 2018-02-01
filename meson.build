project('zim-tools', ['c', 'cpp'],
  version : '0.0.1',
  license : 'GPL2',
  default_options : ['c_std=c11', 'cpp_std=c++11'])

add_global_arguments(['-Werror', '-Wall'], language:'cpp')

static_linkage = get_option('static-linkage')
if static_linkage
  add_global_link_arguments('-static-libstdc++', '--static', language:'cpp')
endif

libzim_dep = dependency('libzim', static:static_linkage)

compiler = meson.get_compiler('cpp')
rt_dep = compiler.find_library('rt', required:false)

subdir('src')
