project('zim-tools', ['c', 'cpp'],
  version : '3.4.0',
  license : 'GPLv3+',
  default_options : ['c_std=c11', 'cpp_std=c++17', 'werror=true'])

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

libzim_dep = dependency('libzim', version:'>=9.1.0', static:static_linkage)
libzim_dep = dependency('libzim', version:'<10.0.0', static:static_linkage)

with_xapian_support = compiler.has_header_symbol('zim/zim.h', 'LIBZIM_WITH_XAPIAN')

find_library_in_compiler = meson.version().version_compare('>=0.31.0')
rt_dep = dependency('rt', required:false)
docopt_dep = dependency('docopt', static:static_linkage)

with_writer = host_machine.system() != 'windows'

if with_writer
  thread_dep = dependency('threads')
  zlib_dep = dependency('zlib', static:static_linkage)
  gumbo_dep = dependency('gumbo', static:static_linkage)
  magic_dep = dependency('libmagic', static:static_linkage, required:false)

  # libmagic.pc has been introduced in version 5.39 of
  # File. Unfortunately Ubuntu 20.04 (Focal) still uses version
  # 5.38. Therefore we have to keep this special handling. As
  # soon as Ubuntu 20.04 will move forward we will be able to remove
  # this custom code. Check https://launchpad.net/ubuntu/+source/file
  # and https://bugs.launchpad.net/ubuntu/+source/file/+bug/1951594
  if not magic_dep.found()
    magic_include_path = ''
    magic_prefix_install = get_option('magic-install-prefix')
    if magic_prefix_install == ''
      if compiler.has_header('magic.h')
        if find_library_in_compiler
          magic_lib = compiler.find_library('magic')
        else
          magic_lib = find_library('magic')
        endif
        magic_dep = declare_dependency(link_args:['-lmagic'])
      else
        error('magic.h not found')
      endif
    else
      if not find_library_in_compiler
        error('For custom magic_prefix_install you need a meson version >=0.31.0')
      endif
      magic_include_path = magic_prefix_install + '/include'
      magic_include_args = ['-I'+magic_include_path]
      if compiler.has_header('magic.h', args:magic_include_args)
        magic_include_dir = include_directories(magic_include_path, is_system:true)
        magic_lib_path = join_paths(magic_prefix_install, get_option('libdir'))
        magic_lib = compiler.find_library('magic', dirs:magic_lib_path, required:false)
        if not magic_lib.found()
          magic_lib_path = join_paths(magic_prefix_install, 'lib')
          magic_lib = compiler.find_library('magic', dirs:magic_lib_path)
        endif
        magic_link_args = ['-L'+magic_lib_path, '-lmagic']
        magic_dep = declare_dependency(include_directories:magic_include_dir, link_args:magic_link_args)
      else
        error('magic.h not found')
      endif
    endif
  endif

endif

subdir('src')
subdir('test')
