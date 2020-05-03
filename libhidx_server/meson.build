subdir('src')

libusb_dep = dependency('libusb-1.0', required: true)

if host_machine.system() != 'windows'
   install_headers(
      'include/libhidx/server/Server.hh',
      'include/libhidx/server/Utils.hh',
      'include/libhidx/server/Buffer.hh',
      subdir: meson.project_name()
   )
endif

libhidx_server_sources = [
   'src/Server.cc',
   'src/Utils.cc'
]

libhidx_server = library(
   'hidx_server',
   version: meson.project_version(),
   sources: [
      libhidx_server_sources
   ],
   include_directories: include_directories('include'),
   dependencies: [
      libusb_dep,
      protobuf_dep,
      buffer_gen_dep
   ],
   install: (host_machine.system() != 'windows')
)
# this will probably require asio_dep in case it's not available locally

libhidx_server_dep = declare_dependency(
   include_directories: include_directories('include'),
   link_with: libhidx_server
)
