project('libhidx', 'cpp', version: '0.0.1', license: 'GPLv3.0+', default_options: 'cpp_std=c++14')

host_os = host_machine.system()
cxx = meson.get_compiler('cpp')

# the original has this but it produces a warning on meson
#if ['gcc', 'clang'].contains(cxx.get_id())
#   add_project_arguments('-Wall', '-Wextra', language: 'cpp')
#endif

if not cxx.has_header('asio.hpp')
   message('Cannot find asio header')
   git = find_program('git', required: true)
   subproject('asio')
   # this is unfinished, we're going to need to do something like:
   # asio_dep = declare_dependency(include_directories('subproject/asio')
endif

protoc = find_program('protoc', required : true)
protobuf_dep = dependency('Protobuf', required: true)

subdir('libhidx_server')
subdir('libhidx')

if host_machine.system() != 'windows'
   subdir('libhidx_server_daemon')
endif
