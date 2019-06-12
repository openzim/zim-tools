project('zim-tools', ['c', 'cpp'],
  version : '1.0.3',
  license : 'GPL2',
  default_options : ['c_std=c11', 'cpp_std=c++11', 'werror=true'])

add_global_arguments(['-Werror', '-Wall'], language:'cpp')

add_global_arguments('-DVERSION="@0@"'.format(meson.project_version()), language : 'cpp')

static_linkage = get_option('static-linkage')
if static_linkage
  add_global_link_arguments('-static-libstdc++', '--static', language:'cpp')
endif

libzim_dep = dependency('libzim', version : '>=5.0.0', static:static_linkage)

compiler = meson.get_compiler('cpp')
rt_dep = compiler.find_library('rt', required:false)

subdir('src')
